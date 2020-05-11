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

//EventQueue queue1(32 * EVENTS_EVENT_SIZE);
EventQueue queue2(32 * EVENTS_EVENT_SIZE);
//Thread check(osPriorityNormal, 16 * 1024);
//Thread music(osPriorityNormal, 16 * 1024);
//Thread play(osPriorityNormal, 16 * 1024);

// I/O
InterruptIn menu(SW2);
InterruptIn btn(SW3);
DigitalOut red_led(LED1);
DigitalOut green_led(LED2);
DigitalOut led(LED3);

int mode = 1;
//bool flag = true;
bool in_menu = false;

constexpr int kTensorArenaSize = 60 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
static tflite::MicroErrorReporter micro_error_reporter;
tflite::ErrorReporter* error_reporter = &micro_error_reporter;
const tflite::Model* model = tflite::GetModel(g_magic_wand_model_data);
static tflite::MicroOpResolver<6> micro_op_resolver;
// Build an interpreter to run the model with
static tflite::MicroInterpreter static_interpreter(
    model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
tflite::MicroInterpreter *interpreter = &static_interpreter;
TfLiteTensor* model_input;
int input_length;

// Whether we should clear the buffer next time we fetch data
bool should_clear_buffer = false;
bool got_data = false;

// The gesture index of the prediction
int gesture_index;

// song data
int song_num = 0;
int step = 0;
char name[3][20] = {"Little Star", "Lightly Row", "Two tigers"};
int song[3][49]; 
int noteLength[3][49];

void mode_sel(void);
void choose(void);
void get_gesture(void);
void forward(void);
void back(void);
void checking(void);
int PredictGesture(float* output);
void playNote(int freq);
void loadSignal(void);

void playing(void)
{
 // while(true) {
    for(step = 0; step < 49; step++) {
      //if (flag == true) {
        uLCD.cls();
        uLCD.printf("\nnow playing\n");
        uLCD.printf("\n%s\n", name[song_num]);
        //uLCD.printf("\n%2D %S %2D\n", song[step], name[song_num ], step);
        int length = noteLength[song_num][step];
        while (length--){
          for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize; ++j) {
            playNote(song[song_num][step]);
            if (!num) num = beat();
          }
          //queue2.call(playNote, song[song_num][step]);
          //if (length <= 1) wait(0.5);
        }
        if (in_menu) break;
      //}
      //else {
        //wait(1.0);
      //}
    }
    //step = 0;
    //if (in_menu) break;
  //}
}

  /*flag = false;
  red_led = 0;
  green_led = 1;
  led = 1;
  uLCD.cls();
  wait(1);
  uLCD.printf("\nMENU:\n");

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
*/
int main(void)
{
    //pc.baud(9600);
    red_led = 1;
    green_led = 1;
    led = 1;

    uLCD.printf("\nstart\n");
    wait(1);
    menu.rise(mode_sel);
    btn.rise(choose);

    uLCD.printf("loading song");
    loadSignal();
    
    checking();

    while (true) {
      if (in_menu) {
        playNote(0);
        if (mode == -1) {
          uLCD.cls();
          uLCD.printf("\nMENU:\n");
            uLCD.printf("\n 1. forward\n");
            uLCD.printf("\n 2. select_song\n");
            uLCD.printf("\n 3. backword\n");
            uLCD.printf("\n 4. Tiako game\n");
          mode++;
        }
        //uLCD.printf("\nmode = %d\n", mode);
        //get_gesture();
          // Attempt to read new data from the accelerometer
        got_data = ReadAccelerometer(error_reporter, model_input->data.f, input_length, should_clear_buffer);
        // If there was no new data,
        // don't try to clear the buffer again and wait until next time
        if (!got_data) {
          should_clear_buffer = false;
          continue;
        }
        // Run inference, and report any error
        TfLiteStatus invoke_status = interpreter->Invoke();
        if (invoke_status != kTfLiteOk) {
          error_reporter->Report("Invoke failed on index: %d\n", begin_index);
          continue;
        }
        // Analyze the results to obtain a prediction
        gesture_index = PredictGesture(interpreter->output(0)->data.f);

        // Clear the buffer next time we read data
        should_clear_buffer = gesture_index < label_num;
        //uLCD.printf("\ngesture = %d\n", gesture_index);
        if (gesture_index == 1) {
          if (mode < 4) mode++;
          else mode = 1;
          
          if (mode == 1) {
            uLCD.cls();
            uLCD.printf("\n1. forward\n");
          }
          if (mode == 2) {
            uLCD.cls();
            uLCD.printf("\n2. select song\n");
          }
          if (mode == 3) {
            uLCD.cls();
            uLCD.printf("\n3. backward\n");
          }
          if (mode == 4) {
            uLCD.cls();
            uLCD.printf("\n4. Tiako game\n");
          }    	
        }
      }	
      else if (mode == 5) {
        playNote(0);
        uLCD.cls();
        for (int i = 0; i < 3; i++) {
          uLCD.printf("No.%d: %s\n", i+1, name[i]);
        }

        while (mode == 5) {
         // get_gesture();
                    // Attempt to read new data from the accelerometer
          got_data = ReadAccelerometer(error_reporter, model_input->data.f, input_length, should_clear_buffer);
          // If there was no new data,
          // don't try to clear the buffer again and wait until next time
          if (!got_data) {
            should_clear_buffer = false;
            continue;
          }
          // Run inference, and report any error
          TfLiteStatus invoke_status = interpreter->Invoke();
          if (invoke_status != kTfLiteOk) {
            error_reporter->Report("Invoke failed on index: %d\n", begin_index);
            continue;
          }
            // Analyze the results to obtain a prediction
          gesture_index = PredictGesture(interpreter->output(0)->data.f);

            // Clear the buffer next time we read data
          should_clear_buffer = gesture_index < label_num;
        
          if (gesture_index == 1){
            if (song_num < 2) song_num++;
            else song_num = 0;
            uLCD.cls();
            uLCD.printf("No.%d: %s\n", song_num+1, name[song_num]);
              /*if (song_num == 0) {
              uLCD.printf("\nNo. 1\n");
            }
            if (song_num == 1) {
              uLCD.printf("\nNo. 2\n");
            }
            if (song_num == 2) {
              uLCD.printf("\nNo. 3\n");
            }  */
          }
        }
      }
      else {
        if (mode != -1){
          uLCD.cls();
          if (mode == 1) {
            uLCD.printf("\nforward\n");
            forward();
            uLCD.printf("No.%d: %s\n", song_num+1, name[song_num]);
            wait(1);
          }
          else if (mode == 3) {
            uLCD.printf("\nbackward\n");
            back();
            uLCD.printf("No.%d: %s\n", song_num+1, name[song_num]);
            wait(1);
          }
          else if (mode == 4) {
            uLCD.printf("\nTiako game\n");
            wait(1);
          }
        }  
        playing();
      }
    
  }

  //check.start(callback(&queue1, &EventQueue::dispatch_forever));
  //menu.fall(queue1.event(mode_sel));
  //play.start(callback(&queue2, &EventQueue::dispatch_forever));
  //music.start(playing);
}

void mode_sel(void)
{
  mode = -1;
  in_menu = true;
}

void choose(void) 
{
  in_menu = false;
  if (mode == 2) mode = 5;
  else if (mode == 5) mode = 6;
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

void get_gesture(void)
{
  // Attempt to read new data from the accelerometer
  got_data = ReadAccelerometer(error_reporter, model_input->data.f, input_length, should_clear_buffer);
  // If there was no new data,
  // don't try to clear the buffer again and wait until next time
  if (!got_data) {
    should_clear_buffer = false;
   // continue;
  }
  // Run inference, and report any error
  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    error_reporter->Report("Invoke failed on index: %d\n", begin_index);
   // continue;
  }
  // Analyze the results to obtain a prediction
  gesture_index = PredictGesture(interpreter->output(0)->data.f);

  // Clear the buffer next time we read data
  should_clear_buffer = gesture_index < label_num;
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
 // static tflite::MicroInterpreter static_interpreter(
   //   model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
  //tflite::MicroInterpreter *interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors
  interpreter->AllocateTensors();

  // Obtain pointer to the model's input tensor
  //TfLiteTensor* model_input = interpreter->input(0);
  model_input = interpreter->input(0);
  
  if ((model_input->dims->size != 4) || (model_input->dims->data[0] != 1) ||
      (model_input->dims->data[1] != config.seq_length) ||
      (model_input->dims->data[2] != kChannelNumber) ||
      (model_input->type != kTfLiteFloat32)) {
    //error_reporter->Report("Bad input tensor parameters in model");
    return -1;
  }
  //int input_length = model_input->bytes / sizeof(float);
  input_length = model_input->bytes / sizeof(float);
  TfLiteStatus setup_status = SetupAccelerometer(error_reporter);

  if (setup_status != kTfLiteOk) {
    error_reporter->Report("Set up failed\n");
    return -1;
  }
  error_reporter->Report("Set up successful...\n");
}

void playNote(int freq)
{
  for(int i = 0; i < kAudioTxBufferSize; i++) {
    waveform[i] = (int16_t) (sin((double)i * 2. * M_PI/(double) (kAudioSampleFrequency / freq)) * ((1<<16) - 1));
  }
  // the loop below will play the note for the duration of 1s
  //for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize; ++j) {
  audio.spk.play(waveform, kAudioTxBufferSize);
  //}
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

void loadSignal(void)
{
  green_led = 0;
  int i = 0;
  int n = 0;
  int serialCount = 0;
  audio.spk.pause();
  //pc.printf("%d\n", song_num);
  for (n = 0; n < 3; n++) {
    i = 0;
    while(i < signalLength)
    {
      if(pc.readable()) {
          serialInBuffer[serialCount] = pc.getc();
          serialCount++;
          if(serialCount == 5) {
              serialInBuffer[serialCount] = '\0';
              song[n][i] = 1000 * (float)atof(serialInBuffer);
              serialCount = 0;
              i++;
          }
      }
    }
  }

  for (n = 0; n < 3; n++) {
    i = 0;
    while(i < signalLength)
    {
      if(pc.readable()) {
        serialInBuffer[serialCount] = pc.getc();
        serialCount++;
        if(serialCount == 5) {
          serialInBuffer[serialCount] = '\0';
          noteLength[n][i] = 1000 * (float)atof(serialInBuffer);
          serialCount = 0;
          i++;
        }
      }
    }
  }
  green_led = 1;
}