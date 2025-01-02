#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <portaudio.h>

#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#define discard (void)

#define PA_SAMPLE_RATE     44100
#define PA_SAMPLES_PER_BUF 64

GLFWwindow* window = NULL;

PaError pa_err = paNoError;

typedef struct {
  bool shift, ctrl, alt, super;
  bool none; // no modifier keys
} KeyMods;

#define G2_FREQ (97.999)

typedef enum {
  // second octave
  NOTE_G2,
  NOTE_G2s,
  NOTE_A2,
  NOTE_A2s,
  NOTE_B2,
  // third octave
  NOTE_C3,
  NOTE_C3s,
  NOTE_D3,
  NOTE_D3s,
  NOTE_E3,
  NOTE_F3,
  NOTE_F3s,
  NOTE_G3,
  NOTE_G3s,
  NOTE_A3,
  NOTE_A3s,
  NOTE_B3,
  // fourth octave
  NOTE_C4,
  NOTE_C4s,
  NOTE_D4,
  NOTE_D4s,
  NOTE_E4,
  NOTE_F4,
  NOTE_F4s,
  NOTE_G4,
  NOTE_G4s,
  __NOTES_NUM__,
  NOTE_BAD = -1
} Note;

typedef struct {
  bool mute;                  // are we muted
  float volume;               // global volume multiplier, [0.0, 1.0]
  float t;                    // time variable for wave calculation
  bool keys[__NOTES_NUM__];   // physical keyboard keys
  float notes[__NOTES_NUM__]; // mapping for notes->frequencies
} Context;
Context g_ctx = {0};

Note keyToNote(int key) {
#define keyToNote(_key, _note) \
  case GLFW_KEY_##_key: return _note;
  switch(key) {
    // second octave
    keyToNote(A, NOTE_G2);
    keyToNote(Z, NOTE_G2s);
    keyToNote(W, NOTE_A2);
    keyToNote(S, NOTE_A2s);
    keyToNote(X, NOTE_B2);
    // third octave
    keyToNote(E, NOTE_C3);
    keyToNote(D, NOTE_C3s);
    keyToNote(C, NOTE_D3);
    keyToNote(R, NOTE_D3s);
    keyToNote(F, NOTE_E3);
    keyToNote(V, NOTE_F3);
    keyToNote(T, NOTE_F3s);
    keyToNote(G, NOTE_G3);
    keyToNote(B, NOTE_G3s);
    keyToNote(Y, NOTE_A3);
    keyToNote(H, NOTE_A3s);
    keyToNote(N, NOTE_B3);
    // fourth octave
    keyToNote(U, NOTE_C4);
    keyToNote(J, NOTE_C4s);
    keyToNote(M, NOTE_D4);
    keyToNote(I, NOTE_D4s);
    keyToNote(K, NOTE_E4);
    case 44: return NOTE_F4; // <
    keyToNote(O, NOTE_F4s);
    keyToNote(L, NOTE_G4);
    case 46: return NOTE_G4s; // >

    default: return NOTE_BAD;
  }
#undef keyToNote
}

static int onAudioCallback(const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *data) {
  discard inputBuffer;
  discard timeInfo;
  discard statusFlags;

  float* out = (float*)outputBuffer;
  Context* ctx = (Context*)data;

  for(unsigned int i = 0; i < framesPerBuffer; i++) {
    if(!ctx->mute) {
      float v = 0;
      float freqSum = 0;
      size_t freqN = 0;
      //float highestFreq = 0;
      for(int i = 0; i < __NOTES_NUM__; i++) {
        if(ctx->keys[i]) {
          float frequency = ctx->notes[i];
          freqSum += frequency;
          freqN++;
          v += sinf(frequency * 2.f*M_PI * ctx->t) / frequency;
        }
      }
      if(freqN == 0) goto zero;
      //v *= 100; // TODO: what should we do here?
      //v *= highestFreq;
      size_t freqN2 = freqN * freqN;
      v *= (freqSum / (float)freqN2);

      v *= ctx->volume;
      *out++ = v; // left
      *out++ = v; // right

      ctx->t += (1.f / (float)PA_SAMPLE_RATE);
      if(ctx->t >= (float)PA_SAMPLE_RATE) ctx->t = 0.f;
    } else {
    zero:
      *out++ = 0.f;
      *out++ = 0.f;
    }
  }

  return paContinue;
}

void onGlfwError(int code, const char* desc) {
  printf("GLFW Error (0x%08x): %s\n", code, desc);
}

void onKey(GLFWwindow* w, int key, int _, int action, int mods_i) {
  if(action == GLFW_REPEAT) return;
  bool down = action == GLFW_PRESS;
  discard _;
  discard w;
  KeyMods mods = {
    .shift = mods_i & GLFW_MOD_SHIFT,
    .ctrl = mods_i & GLFW_MOD_CONTROL,
    .alt = mods_i & GLFW_MOD_ALT,
    .super = mods_i & GLFW_MOD_SUPER,
    .none = mods_i == 0
  };

  // exiting with C-q
  if(down && mods.ctrl && key == GLFW_KEY_Q)
    glfwSetWindowShouldClose(window, GLFW_TRUE);

  // toggle mute with backslash
  if(down && key == GLFW_KEY_BACKSLASH)
    g_ctx.mute ^= 1;

  // adjusting volume with C-up/down
  if(!down && (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN)) {
    float volMod = 0.1f;
    if(key == GLFW_KEY_DOWN) volMod *= -1;
    if(mods.ctrl) volMod *= 0.1f; // 0.01f
    g_ctx.volume += volMod;
    if(g_ctx.volume > 1.f) {
      g_ctx.volume = 1.f;
      printf("Changed volume to 100%%\n");
    } else if (g_ctx.volume < 0.01f)
      g_ctx.volume = 0;
    else // everything OK, notify the user
      printf("Changed volume to %d%%\n", (int)(100*g_ctx.volume));
  }

  // playing notes
  if(mods.none) {
    Note note = keyToNote(key);
    if(note != NOTE_BAD)
      g_ctx.keys[note] = down;
  }

  /* if(down)
    printf("Pressed key %d\n", key);
  else
    printf("Released key %d\n", key);*/
}

#define paCheckError() \
  if(pa_err != paNoError) goto pa_error;

int main() {
  if(!glfwInit()) {
    printf("Failed to initiate GLFW\n");
    return 1;
  }
  printf("Using GLFW %s\n", glfwGetVersionString());

  float tuningValue = powf(2.f, 1.f / 12.f); // 12th root of 2
  { // generate note->frequency table
    float mult = 1;
    for(size_t i = 0; i < __NOTES_NUM__; i++) {
      g_ctx.notes[i] = G2_FREQ * mult;
      mult *= tuningValue;
    }
  }

  PaStream* stream = NULL;
  { // initalize portaudio
    pa_err = Pa_Initialize();
    paCheckError();

    PaStreamParameters pa_params = {0};
    pa_params.device = Pa_GetDefaultOutputDevice();
    if(pa_params.device == paNoDevice) {
      printf("PortAudio error: no default output device!\n");
      goto pa_error;
    }

    pa_params.channelCount = 2;         // stereo output
    pa_params.sampleFormat = paFloat32; // 32 bit floating point output
    pa_params.suggestedLatency =
      Pa_GetDeviceInfo( pa_params.device )->defaultLowOutputLatency;
    pa_params.hostApiSpecificStreamInfo = NULL;

    pa_err = Pa_OpenStream(&stream, NULL, &pa_params, PA_SAMPLE_RATE, PA_SAMPLES_PER_BUF,
                           paNoFlag, &onAudioCallback, &g_ctx);
    paCheckError();
  }

  glfwSetErrorCallback(onGlfwError);

  window = glfwCreateWindow(800, 600, "Bayan.c", NULL, NULL);
  if(!window) {
    printf("Failed to create a window\n");
    return 1;
  }
  glfwMakeContextCurrent(window);

  if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    printf("Failed to init GLAD\n");
    return 1;
  }

  printf("Using OpenGL %s\n", glGetString(GL_VERSION));

  glfwSetKeyCallback(window, onKey);

  g_ctx.volume = 0.95;
  printf("Starting Bayan with inital volume %d%%\n", (int)(g_ctx.volume*100));
  pa_err = Pa_StartStream(stream);
  paCheckError();

  float c = 0;
  bool dec = false;
  while(!glfwWindowShouldClose(window)) {
    glClearColor(c, 0.5, 1-c, 1); // basic screen color effects
    glClear(GL_COLOR_BUFFER_BIT);
    if(c >= 1.0f) dec = true;
    if(c <= 0.0f) dec = false;
    if(!dec) c += 1.0f/255.0f;
    else c -= 1.0f/255.0f;
    //g_ctx.freq = 90 + c * 100;
    //g_ctx.volume = c;

    glfwPollEvents();
    glfwSwapBuffers(window);
  }
  // cleanup
  Pa_StopStream(stream);
  Pa_CloseStream(stream);
  Pa_Terminate();
  glfwTerminate();
  return 0;

 pa_error:
  Pa_Terminate();
  glfwTerminate();
  printf("PortAudio error has occurred:\n"
         " %s (0x%x)\n", Pa_GetErrorText(pa_err), pa_err);
  return 1;
}
