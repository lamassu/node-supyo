#include <cstdlib>
#include <ctime>
#include <nan.h>

#include "pico/picort.c"

/*
  object detection parameters
*/

#ifndef SCALEFACTOR
#define SCALEFACTOR 1.2f
#endif

#ifndef STRIDEFACTOR
#define STRIDEFACTOR 0.1f
#endif

using namespace v8;

NAN_METHOD(Detect) {
  Local<Object> bufferObj = info[0].As<v8::Object>();
  uint8_t* pixels = (uint8_t *)node::Buffer::Data(bufferObj);
//  size_t        npixels = Buffer::Length(bufferObj);

  int32_t ncols = info[1]->IntegerValue();
  int32_t nrows = info[2]->IntegerValue();
  int32_t minsize = info[3]->IntegerValue();
  float cutoff = (float)info[4]->NumberValue();

  #define MAXNDETECTIONS 2048
  int ndetections;
  float qs[MAXNDETECTIONS], rs[MAXNDETECTIONS],
    cs[MAXNDETECTIONS], ss[MAXNDETECTIONS];

  // a structure that encodes object appearance
  static unsigned char appfinder[] = {
    #include "pico/facefinder.ea"
  };

  // scan the image at 4 different orientations
  ndetections = 0;
  int min_dimension = nrows < ncols ? nrows : ncols;

  for (int i = 0; i < 4; ++i) {
    float orientation = i*2*3.14f/4;

    ndetections += find_objects(orientation, &rs[ndetections], &cs[ndetections],
      &ss[ndetections], &qs[ndetections], MAXNDETECTIONS-ndetections, appfinder,
      pixels, nrows, ncols, ncols, SCALEFACTOR, STRIDEFACTOR, minsize,
      min_dimension, 1);
  }

  bool detected = false;
  for (int i = 0; i < ndetections; ++i) {
    if(qs[i] >= cutoff) {
      detected = true;
      break;
    }
  }

  info.GetReturnValue().Set(Nan::New(detected));
}

NAN_MODULE_INIT(InitAll) {
  Nan::Set(target, Nan::New("detect").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(Detect)).ToLocalChecked());
}

NODE_MODULE(supyo, InitAll);
