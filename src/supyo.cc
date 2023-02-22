#include <cstdlib>
#include <ctime>
#include <nan.h>

#include "pico/picort.c"

/*
 * a portable time function
 */

#ifdef __GNUC__
#include <time.h>
float getticks()
{
	struct timespec ts;

	if(clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
		return -1.0f;

	return ts.tv_sec + 1e-9f*ts.tv_nsec;
}
#else
#include <windows.h>
float getticks()
{
	static double freq = -1.0;
	LARGE_INTEGER lint;

	if(freq < 0.0)
	{
		if(!QueryPerformanceFrequency(&lint))
			return -1.0f;

		freq = lint.QuadPart;
	}

	if(!QueryPerformanceCounter(&lint))
		return -1.0f;

	return (float)( lint.QuadPart/freq );
}
#endif

/*
 * object detection parameters
 */

/**
 * how much to rescale the window during the multiscale detection process
 * increasing this value leads to lower number of detections and higher processing
 * speed for example, set to 1.2f if you're using pico on a mobile device
 */
#ifndef SCALEFACTOR
#define SCALEFACTOR 1.1f
#endif

/**
 * how much to move the window between neighboring detections increasing this
 * value leads to lower number of detections and higher processing speed
 * for example, set to 0.05f if you want really high recall
 */
#ifndef STRIDEFACTOR
#define STRIDEFACTOR 0.1f
#endif

/**
 * max number of detected objects
 */
#define MAXNDETECTIONS 2048

using namespace v8;

/**
 * @param {uint8_t*} buffer - byte array of the greyscale image
 * @param {number}   ncols  - image width (in pixels)
 * @param {number}   nrows  - image height (in pixels)
 * @param {number}   minsize - face minimum size (in pixels)
 * @param {number}   cutoff - quality threshold (suggested 0.5)
 * @param {boolean}  verbose - print debug messages to stdout
 */
NAN_METHOD(Detect) {
  // greyscale image
  Local<Object> bufferObj = info[0].As<v8::Object>();
  uint8_t* pixels = (uint8_t *)node::Buffer::Data(bufferObj);
  // image width
  int32_t ncols   = info[1]->IntegerValue();
  // image height
  int32_t nrows   = info[2]->IntegerValue();
  // sets the minimum size (in pixels) of an object - suggested 128
  int32_t minsize = info[3]->IntegerValue();

  // detection quality threshold (must be >= 0.0f)
 	// you can vary the TPR and FPR with this value
 	// if you're experiencing too many false positives
  // try a larger number here (for example, 7.5f)
  float cutoff    = (float)info[4]->NumberValue();

  // print debug messages to stdout
  bool verbose    = (bool)info[5]->BooleanValue();

  if (verbose) {
    size_t npixels = node::Buffer::Length(bufferObj);
    fprintf(stderr, "[supyo] image %d x %d = %zu pixes\n", ncols, nrows, npixels);
  }

  int ndetections = 0, i;
  float t;
  float orientation;
  float rcsq[4*MAXNDETECTIONS];
  bool detected = false;

  // work out the number width step
  int n_channels = 1;
  int size_row_raw = ncols * n_channels;
  int rem = size_row_raw % 4;
  int width_step = (rem == 0) ? size_row_raw : size_row_raw + rem;

  if (verbose) {
    fprintf(stderr, "[supyo] width_step %d\n", width_step);
  }

	// perform detection with the pico library
	t = getticks();

  // a structure that encodes object appearance
  static unsigned char appfinder[] = {
    #include "pico/facefinder.ea"
  };

  for (i = 1; i <= 4; i++) {
    // `orientation` is a number between 0 and 1 that determines the counterclockwise
    // in-plane rotation of the cascade: 0.0f corresponds to 0 radians
    // and 1.0f corresponds to 2*pi radians
    //orientation = i * 2 * 3.14f / 4;
    orientation = i / 4.f;

    ndetections += find_objects(rcsq,
      MAXNDETECTIONS - ndetections,
      appfinder,
      orientation,
      pixels,
      nrows, ncols, width_step,
      SCALEFACTOR, STRIDEFACTOR, minsize,
      MIN(nrows, ncols));

    if (verbose) {
      fprintf(stderr, "[supyo] orientation %f ndetections %d\n", orientation * 2 * 3.14f, ndetections);
    }
  }

  ndetections = cluster_detections(rcsq, ndetections);
  if (verbose) {
    fprintf(stderr, "[supyo] cluster detections %d\n", ndetections);
  }

  for (i = 0; i < ndetections; ++i) {
    // check the confidence threshold
    if(rcsq[4*i+3] >= cutoff) {
      if (verbose) {
        fprintf(stderr, "[supyo] face detected at (x=%d, y=%d, r=%d) confidence %f\n", (int)rcsq[4*i+0], (int)rcsq[4*i+1], (int)rcsq[4*i+2], rcsq[4*i+3]);
      }

      detected = true;
      break;
    } else if (verbose) {
      fprintf(stderr, "[supyo] result confidence %f < threshold (%f)\n", rcsq[4*i+3], cutoff);
    }
  }

  if (verbose) {
    t = getticks() - t;
    fprintf(stderr, "[supyo] time taken %f\n", 1000.0f * t);
  }

  info.GetReturnValue().Set(Nan::New(detected));
}

NAN_MODULE_INIT(InitAll) {
  Nan::Set(
    target,
    Nan::New("detect").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(Detect)).ToLocalChecked()
  );
}

NODE_MODULE(supyo, InitAll);
