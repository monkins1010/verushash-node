#include <nan.h>
#include <node.h>
#include <node_buffer.h>
#include <v8.h>
#include <stdint.h>
#include <vector>

#include "crypto/verus_hash.h"
#include "sodium.h"

using namespace v8;

using namespace node;
using namespace v8;

#define THROW_ERROR_EXCEPTION(x) Nan::ThrowError(x)
const char *ToCString(const Nan::Utf8String &value)
{
  return *value ? *value : "<string conversion failed>";
}

// Verushash Algorithm
CVerusHash *vh;
CVerusHashV2 *vh2;
CVerusHashV2 *vh2b1;
CVerusHashV2 *vh2b2;

bool initialized = false;

void initialize()
{
  if (!initialized)
  {
    CVerusHash::init();
    CVerusHashV2::init();
  }

  vh = new CVerusHash();
  vh2 = new CVerusHashV2(SOLUTION_VERUSHHASH_V2);
  vh2b1 = new CVerusHashV2(SOLUTION_VERUSHHASH_V2_1);
  vh2b2 = new CVerusHashV2(SOLUTION_VERUSHHASH_V2_2);

  initialized = true;
}

void verusInit(const v8::FunctionCallbackInfo<Value> &args)
{
  initialize();
  args.GetReturnValue().Set(args.This());
}

NAN_METHOD(hash)
{
  // Check Arguments for Errors
  if (info.Length() < 1)
    return THROW_ERROR_EXCEPTION("You must provide one argument.");

  // Process/Define Passed Parameters
  char *input = Buffer::Data(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  uint32_t input_len = Buffer::Length(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  char output[32];

  if (initialized == false)
  {
    initialize();
  }

  verus_hash(output, input, input_len);
  info.GetReturnValue().Set(Nan::CopyBuffer(output, 32).ToLocalChecked());
}

NAN_METHOD(hashV2)
{
  // Check Arguments for Errors
  if (info.Length() < 1)
    return THROW_ERROR_EXCEPTION("You must provide one argument.");

  // Process/Define Passed Parameters
  char *input = Buffer::Data(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  uint32_t input_len = Buffer::Length(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  char output[32];

  if (initialized == false)
  {
    initialize();
  }

  vh2->Reset();
  vh2->Write((const unsigned char *)input, input_len);
  vh2->Finalize((unsigned char *)output);
  info.GetReturnValue().Set(Nan::CopyBuffer(output, 32).ToLocalChecked());
}

NAN_METHOD(hashV2b)
{
  // Check Arguments for Errors
  if (info.Length() < 1)
    return THROW_ERROR_EXCEPTION("You must provide one argument.");

  // Process/Define Passed Parameters
  char *input = Buffer::Data(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  uint32_t input_len = Buffer::Length(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  char output[32];

  if (initialized == false)
  {
    initialize();
  }

  vh2->Reset();
  vh2->Write((const unsigned char *)input, input_len);
  vh2->Finalize2b((unsigned char *)output);
  info.GetReturnValue().Set(Nan::CopyBuffer(output, 32).ToLocalChecked());
}

NAN_METHOD(hashV2b1)
{
  // Check Arguments for Errors
  if (info.Length() < 1)
    return THROW_ERROR_EXCEPTION("You must provide one argument.");

  // Process/Define Passed Parameters
  char *input = Buffer::Data(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  uint32_t input_len = Buffer::Length(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  char output[32];

  if (initialized == false)
  {
    initialize();
  }

  vh2b1->Reset();
  vh2b1->Write((const unsigned char *)input, input_len);
  vh2b1->Finalize2b((unsigned char *)output);
  info.GetReturnValue().Set(Nan::CopyBuffer(output, 32).ToLocalChecked());
}

const unsigned char BLAKE2Bpersonal[crypto_generichash_blake2b_PERSONALBYTES] = {'V', 'e', 'r', 'u', 's', 'D', 'e', 'f', 'a', 'u', 'l', 't', 'H', 'a', 's', 'h'};
uint256 blake2b_hash(unsigned char *data, unsigned long long length)
{
  const unsigned char *personal = BLAKE2Bpersonal;
  crypto_generichash_blake2b_state state;
  uint256 result;
  if (crypto_generichash_blake2b_init_salt_personal(
          &state,
          NULL, 0, // No key.
          32,
          NULL, // No salt.
          personal) == 0)
  {
    crypto_generichash_blake2b_update(&state, data, length);
    if (crypto_generichash_blake2b_final(&state, reinterpret_cast<unsigned char *>(&result), crypto_generichash_blake2b_BYTES) == 0)
    {
      return result;
    }
  }
  result.SetNull();
  return result;
}

NAN_METHOD(hashV2b2)
{
  // Check Arguments for Errors
  if (info.Length() < 1)
    return THROW_ERROR_EXCEPTION("You must provide one argument.");

  // Process/Define Passed Parameters
  char *input = Buffer::Data(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  uint32_t input_len = Buffer::Length(Nan::To<v8::Object>(info[0]).ToLocalChecked());
  char output[32];

  if (initialized == false)
  {
    initialize();
  }

  // detect pbaas, validate and clear non-canonical data if needed
  char *solution = (input + 140 + 3);
  unsigned int sol_ver = ((solution[0]) + (solution[1] << 8) + (solution[2] << 16) + (solution[3] << 24));
  if (sol_ver > 6)
  {
    // const uint8_t descrBits = solution[4];
    const uint8_t numPBaaSHeaders = solution[5];
    // const uint16_t extraSpace = solution[6] | ((uint16_t)(solution[7]) << 8);
    const uint32_t soln_header_size = 4 + 1 + 1 + 2 + 32 + 32; // version, descr, numPBaas, extraSpace, hashPrevMMRroot, hashBlockMMRroot
    const uint32_t soln_pbaas_cid_size = 20;                   // hash160
    const uint32_t soln_pbaas_prehash_sz = 32;                 // pre header hash blake2b
    // if pbaas headers present
    if (numPBaaSHeaders > 0)
    {
      unsigned char preHeader[32 + 32 + 32 + 32 + 4 + 32 + 32] = {
          0,
      };

      // copy non-canonical items from block header
      memcpy(&preHeader[0], input + 4, 32);                         // hashPrevBlock
      memcpy(&preHeader[32], input + 4 + 32, 32);                   // hashMerkleRoot
      memcpy(&preHeader[64], input + 4 + 32 + 32, 32);              // hashFinalSaplingRoot
      memcpy(&preHeader[96], input + 4 + 32 + 32 + 32 + 4 + 4, 32); // nNonce (if nonce changes must update preHeaderHash in solution)
      memcpy(&preHeader[128], input + 4 + 32 + 32 + 32 + 4, 4);     // nbits
      memcpy(&preHeader[132], solution + 8, 32 + 32);              // hashPrevMMRRoot, hashPrevMMRRoot

      // detect if merged mining is present and clear non-canonical data (if needed)
      int matched_zeros = 0;
      for (int i = 0; i < sizeof(preHeader); i++)
      {
        if (preHeader[i] == 0)
        {
          matched_zeros++;
        }
      }

      // if the data has already been cleared of non-canonical data, just continue along
      if (matched_zeros != sizeof(preHeader))
      {
        // detect merged mining by looking for preHeaderHash (blake2b) in first pbaas chain definition
        int matched_hashes = 0;
        uint256 preHeaderHash = blake2b_hash(&preHeader[0], sizeof(preHeader));
        if (!preHeaderHash.IsNull())
        {
          if (memcmp((unsigned char *)&preHeaderHash,
                     &solution[soln_header_size + soln_pbaas_cid_size],
                     soln_pbaas_prehash_sz) == 0)
          {
            matched_hashes++;
          }
        }
        // clear non-canonical data for pbaas merge mining
        if (matched_hashes > 0)
        {
          memset(input + 4, 0, 32 + 32 + 32);              // hashPrevBlock, hashMerkleRoot, hashFinalSaplingRoot
          memset(input + 4 + 32 + 32 + 32 + 4, 0, 4);      // nBits
          memset(input + 4 + 32 + 32 + 32 + 4 + 4, 0, 32); // nNonce
          memset(solution + 8, 0, 32 + 32);               // hashPrevMMRRoot, hashBlockMMRRoot
                                                          // printf("info: merged mining %d chains, clearing non-canonical data on hash found\n", numPBaaSHeaders);
        }
        else
        {
          // invalid share, pbaas activated must be pbaas mining capatible
          memset(output, 0xff, 32);
          info.GetReturnValue().Set(Nan::NewBuffer(output, 32).ToLocalChecked());
          return;
        }
      }
      else
      {
        // printf("info: merged mining %d chains, non-canonical data pre-cleared\n", numPBaaSHeaders);
      }
    }
  }

  vh2b2->Reset();
  vh2b2->Write((const unsigned char *)input, input_len);
  vh2b2->Finalize2b((unsigned char *)output);
  info.GetReturnValue().Set(Nan::CopyBuffer(output, 32).ToLocalChecked());
}

NAN_MODULE_INIT(init)
{
  Nan::Set(target, Nan::New("hash").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(hash)).ToLocalChecked());
  Nan::Set(target, Nan::New("hashV2").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(hashV2)).ToLocalChecked());
  Nan::Set(target, Nan::New("hash2b").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(hashV2b)).ToLocalChecked());
  Nan::Set(target, Nan::New("2b1").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(hashV2b1)).ToLocalChecked());
  Nan::Set(target, Nan::New("hash2b2").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(hashV2b2)).ToLocalChecked());

}

NODE_MODULE(verushash, init)
