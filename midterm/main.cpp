#include "mbed.h"
#include "string.h"
// uLCD
#include "uLCD_4DGL.h"
// DNN, ACC
#include "accelerometer_handler.h"
#include "config.h"
#include "magic_wand_model_data.h"

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
// audio
#include <cmath>
#include "DA7212.h"

#define bufferLength (32)
#define signalLength (49)

DA7212 audio;
int16_t waveform[kAudioTxBufferSize];
char serialInBuffer[bufferLength];

Serial pc(USBTX, USBRX);
uLCD_4DGL uLCD(D1, D0, D2);

EventQueue queue1(32 * EVENTS_EVENT_SIZE);
EventQueue queue2(32 * EVENTS_EVENT_SIZE);
Thread check(osPriorityNormal, 16 * 1024);
Thread music(osPriorityNormal, 16 * 1024);
Thread play(osPriorityNormal, 16 * 1024);

// I/O
InterruptIn menu(SW2);
DigitalIn btn(SW3);
DigitalOut red_led(LED1);
DigitalOut green_led(LED2);
DigitalOut led(LED3);

int mode = 2;
bool flag = true;

constexpr int kTensorArenaSize = 60 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
static tflite::MicroErrorReporter micro_error_reporter;
tflite::ErrorReporter* error_reporter = &micro_error_reporter;
const tflite::Model* model = tflite::GetModel(g_magic_wand_model_data);
static tflite::MicroOpResolver<6> micro_op_resolver;

// song data
int song_num;
int step = 0;
char name[3][20] = {"Little Star", "Lightly Row", "Two tigers"};
int song[49] = {
    261, 261, 392, 392, 440, 440, 392,
     349, 349, 330, 330, 294, 294, 261,
     392, 392, 349, 349, 330, 330, 294,
     392, 392, 349, 349, 330, 330, 294,
     261, 261, 392, 392, 440, 440, 392,
     349, 349, 330, 330, 294, 294, 261,
     0, 0, 0, 0, 0, 0, 0
     

     /*{392, 330, 330, 349, 294, 294,
      262, 294, 330, 349, 392, 392, 392,
      392, 330, 330, 349, 294, 294,
      261, 330, 392, 392, 330,
      294, 294, 294, 294, 294, 330, 349,
      330, 330, 330, 330, 330, 349, 392,
      392, 330, 330, 349, 294, 294,
      261, 330, 392, 392, 261
     },

     {261, 293, 330, 261, 261, 293, 330, 261, 
     330, 349, 392, 329, 349, 392,
     392, 440, 392, 349, 330, 261,
     392, 440, 392, 349, 330, 261,
     293, 196, 261, 293, 196, 261,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0
     }*/
};

int noteLength[49] = {
    1, 1, 1, 1, 1, 1, 2,
     1, 1, 1, 1, 1, 1, 2,
     1, 1, 1, 1, 1, 1, 2,
     1, 1, 1, 1, 1, 1, 2,
     1, 1, 1, 1, 1, 1, 2,
     1, 1, 1, 1, 1, 1, 2,
     0,0,0,0,0,0,0
    

    /*{1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 3,
    1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 3 
    },

    {1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1
    }*/
};

void playNote(int freq)
{
  for(int i = 0; i < kAudioTxBufferSize; i++) {
    waveform[i] = (int16_t) (sin((double)i * 2. * M_PI/(double) (kAudioSampleFrequency / freq)) * ((1<<16) - 1));
  }
  // the loop below will play the note for the duration of 1s
  for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize; ++j) {
    audio.spk.play(waveform, kAudioTxBufferSize);
  }
}

void forward(void)
{
    if (song_num < 2) song_num++;
    else song_num = 0;
}

void back(void)
{
    if (song_num) song_num--;
    else song_num = 2;
}

void loadSignal(void)
{
  green_led = 0;
  int i = 0;
  int serialCount = 0;
  //audio.spk.pause();
  pc.printf("%d\n", song_num);
  while(i < signalLength)
  {
    if(pc.readable()) {
        serialInBuffer[serialCount] = pc.getc();
        serialCount++;
        if(serialCount == 3) {
            serialInBuffer[serialCount] = '\0';
            song[i] = (float)atof(serialInBuffer);
            serialCount = 0;
            i++;
        }
    }
  }
  i = 0;
  while(i < signalLength)
  {
    if(pc.readable()) {
        serialInBuffer[serialCount] = pc.getc();
        serialCount++;
        if(serialCount == 1) {
            serialInBuffer[serialCount] = '\0';
            song[i] = (float)atof(serialInBuffer);
            serialCount = 0;
            i++;
        }
    }
  }
  green_led = 1;
}

// Return the result of the last prediction
int PredictGesture(float* output) 
{
  // How many times the most recent gesture has been matched in a row
  static int continuous_count = 0;
  // The result of the last prediction
  static int last_predict = -1;

  // Find whichever output has a probability > 0.8 (they sum to 1)
  int this_predict = -1;
  for (int i = 0; i < label_num; i++) {
    if (output[i] > 0.8) this_predict = i;
  }

  // No gesture was detected above the threshold
  if (this_predict == -1) {
    continuous_count = 0;
    last_predict = label_num;
    return label_num;
  }

  if (last_predict == this_predict) {
    continuous_count += 1;
  } 
  else {
    continuous_count = 0;
  }
  last_predict = this_predict;

  // If we haven't yet had enough consecutive matches for this gesture,
  // report a negative result
  if (continuous_count < config.consecutiveInferenceThresholds[this_predict]) {
    return label_num;
  }
  // Otherwise, we've seen a positive result, so clear all our variables
  // and report it
  continuous_count = 0;
  last_predict = -1;

  return this_predict;
}

void checking(void)
{
  micro_op_resolver.AddBuiltin(
      tflite::BuiltinOperator_DEPTHWISE_CONV_2D,
      tflite::ops::micro::Register_DEPTHWISE_CONV_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_MAX_POOL_2D,
                               tflite::ops::micro::Register_MAX_POOL_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_CONV_2D,
                               tflite::ops::micro::Register_CONV_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_FULLY_CONNECTED,
                               tflite::ops::micro::Register_FULLY_CONNECTED());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_SOFTMAX,
                               tflite::ops::micro::Register_SOFTMAX());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_RESHAPE,
                               tflite::ops::micro::Register_RESHAPE(), 1);
  
  // Build an interpreter to run the model with
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
  tflite::MicroInterpreter *interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors
  interpreter->AllocateTensors();

  // Obtain pointer to the model's input tensor
  TfLiteTensor* model_input = interpreter->input(0);

  if ((model_input->dims->size != 4) || (model_input->dims->data[0] != 1) ||
      (model_input->dims->data[1] != config.seq_length) ||
      (model_input->dims->data[2] != kChannelNumber) ||
      (model_input->type != kTfLiteFloat32)) {
    //error_reporter->Report("Bad input tensor parameters in model");
    return -1;
  }
  int input_length = model_input->bytes / sizeof(float);

  TfLiteStatus setup_status = SetupAccelerometer(error_reporter);

  if (setup_status != kTfLiteOk) {
    //error_reporter->Report("Set up failed\n");
    return -1;
  }
  //error_reporter->Report("Set up successful...\n");

  TfLiteTensor *model_input1 = model_input;
  tflite::MicroInterpreter *interpreter1 = interpreter;
  
  // Whether we should clear the buffer next time we fetch data
  bool should_clear_buffer = false;
  bool got_data = false;

  // The gesture index of the prediction
  int gesture_index;
  
  int input_length1 = input_length;


  while (true) {
    if (!btn) break;
    else {
      // Attempt to read new data from the accelerometer
      got_data = ReadAccelerometer(error_reporter, model_input1->data.f, input_length1, should_clear_buffer);
      //got_data = ReadAccelerometer(model_input1->data.f, input_length1, should_clear_buffer);
      // If there was no new data,
      // don't try to clear the buffer again and wait until next time
  	  if (!got_data) {
        should_clear_buffer = false;
     	  continue;
      }
     	// Run inference, and report any error
    	TfLiteStatus invoke_status = interpreter1->Invoke();
    	if (invoke_status != kTfLiteOk) continue;

    	// Analyze the results to obtain a prediction
    	gesture_index = PredictGesture(interpreter->output(0)->data.f);

    	// Clear the buffer next time we read data
    	should_clear_buffer = gesture_index < label_num;

      if (gesture_index == 1) {
        if (mode == 0) {
          uLCD.printf("\nforward\n");
        }
        if (mode == 1) {
          uLCD.printf("\nchange song\n");
        }
        if (mode == 2) {
          uLCD.printf("\nbackward\n");
        }   	
        if (mode < 2) mode++;
        else mode = 0;
      }
    }	
  }
}

void playing(void)
{
  while(true) {
    for(step = 0; step < 49; step++) {
      if (flag == true) {
        uLCD.cls();
        uLCD.printf("\nplaying\n");
        uLCD.printf("\n%s\n", name[song_num]);
        //uLCD.printf("\n%2D %S %2D\n", song[step], name[song_num ], step);
        int length = noteLength[step];
        while (length--){
          queue2.call(playNote, song[step]);
          if (length <= 1) wait(0.5);
        }
      }
      else {
        wait(1.0);
      }
    }
    step = 0;
  }
}

void mode_sel(void)
{
  flag = false;
  red_led = 0;
  green_led = 1;
  led = 1;
  uLCD.cls();
  wait(1);
  uLCD.printf("\nMENU:\n");
  uLCD.printf("\n backward\n");
  uLCD.printf("\n forward\n");
  uLCD.printf("\n change song\n");

  while (true) {
    wait(0.5);
    checking();
    red_led = 1;
    green_led = 1;
    led = 1;   
    uLCD.cls();
    if (mode == 0){
      uLCD.printf("\nbackward\n");
      back();
      uLCD.printf("\nsong: %s", name[song_num]);
      step = 0;
      loadSignal();
      wait(1);
      break;
    } 
    if (mode == 1){
      uLCD.printf("\nforward\n");
      forward();
      uLCD.printf("\nsong: %s", name[song_num]);
      step = 0;
      loadSignal();
      wait(1);
      break;
    } 
    if (mode == 2){
      uLCD.printf("\nselect song\n");
      while (true) {
        if (!btn) {
          if (song_num < 2) song_num++;
          else song_num = 0;
          uLCD.cls();
          uLCD.printf("\nsong: %s", name[song_num]);
          wait(1);
        }
        else {
          uLCD.printf("\nsong: %s", name[song_num]);
          step = 0;
          loadSignal();
          wait(1);
          break;
        }
      }
    }
  }
}

int main(void)
{
    pc.baud(9600);
    red_led = 1;
    green_led = 1;
    led = 1;

    uLCD.printf("\nstart\n");
    wait(1);

    check.start(callback(&queue1, &EventQueue::dispatch_forever));
    menu.fall(queue1.event(mode_sel));
    play.start(callback(&queue2, &EventQueue::dispatch_forever));
    music.start(playing);
}

