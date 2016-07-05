#include <Bounce2.h>

int sequence[] = {5, 6, 7, 8, 9, 10, 11, 12,
                  A0, A1, A2, A3, A4, A5};

                 //{A5, A4, A3, A2, A1, A0, 
                 //12, 11, 10,  9,  8,  7,  6,  5};

const size_t max_sequence = sizeof(sequence)/sizeof(int);

int sequence_position = 0;

const int coil_time = 15; // sec

const int saturn_time = 25; // sec TBD
const int brick_time = 45; // sec

enum state_enum {NO_STATE, TIMING_STATE, TOFIRE_STATE, FIRING_STATE} all_state = NO_STATE;

int to_fire = -1;

Bounce fire_state = Bounce();
Bounce mode_state = Bounce();
Bounce arm_state  = Bounce();

#define FIRE_INTERVAL 100
#define MODE_INTERVAL 100
#define ARM_INTERVAL  500

#define FIRE_PIN 3
#define ARM_PIN  2
#define MODE_PIN 4
#define LED_PIN 13

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);  

  Serial.println("Startup...");
  // We want these to be inputs, rather than outputs
  pinMode(FIRE_PIN, INPUT);
  pinMode(MODE_PIN, INPUT);
  pinMode(ARM_PIN,  INPUT);

  
  // Hi-Z, 20k pull up
  digitalWrite(FIRE_PIN, HIGH);
  digitalWrite(MODE_PIN, HIGH);
  // Hi-Z, no pull up, will read low, we can add a pull down if needed
  digitalWrite(ARM_PIN, LOW);

  fire_state.attach(FIRE_PIN);
  mode_state.attach(MODE_PIN);
  arm_state.attach(ARM_PIN);  

  fire_state.interval(FIRE_INTERVAL);
  mode_state.interval(MODE_INTERVAL);
  arm_state.interval(ARM_INTERVAL);

  // set all the pins as high z inputs, they'll be outputs when we fire them
  for(int p = 0; p<max_sequence; p++) {
    Serial.print("Setting pin ");
    Serial.print(sequence[p], DEC);
    Serial.println(" as Hi-Z input");
    pinMode(sequence[p], INPUT);
    digitalWrite(sequence[p], HIGH);
  }
}

void timing_sm() {
  static unsigned long last_trigger = 0;
  unsigned long wait_time = mode_state.read() == HIGH ? brick_time * (long) 1000: saturn_time * (long) 1000;

  unsigned long diff_time = millis() - last_trigger;

/*  Serial.print("Last Trigger: ");
  Serial.print(last_trigger, DEC);
  Serial.print(" diff_time: ");
  Serial.print(diff_time, DEC);
  Serial.print(" wait_time: ");
  -Serial.println(wait_time); */

  if ((diff_time > wait_time) || last_trigger == 0) {
    last_trigger = millis();

    Serial.println("Time up, firing");

    all_state = TOFIRE_STATE;
    to_fire = sequence[sequence_position++];
    if (sequence_position == 16) {
      to_fire = -1;
      all_state = NO_STATE;
    }
  }
}

void firing_sm() {
  static unsigned long last_fire = 0;

  switch(all_state) {
    case TOFIRE_STATE:
      last_fire = millis();
      Serial.println("fired");
      pinMode(to_fire, OUTPUT);
      digitalWrite(to_fire, LOW);
      all_state = FIRING_STATE;
      break;
    case FIRING_STATE:
      if (millis() - last_fire > coil_time * 1000) {
        digitalWrite(to_fire, HIGH); // drive to 5V
        pinMode(to_fire, INPUT); // Hi-Z
        digitalWrite(to_fire, HIGH); // with 20k pull up
        all_state = TIMING_STATE;
        Serial.println("done firing");
      }
      break;
    default:
      all_state = NO_STATE;
  }  
}

void debug_state() {
  Serial.print("MODE: ");
  Serial.print(mode_state.read() == HIGH ? "brick" : "saturn");
  Serial.print(" STATE: ");
  switch(all_state) {
    case NO_STATE:
      Serial.println("IDLE STATE");
      break;
    case TIMING_STATE:
      Serial.println("TIMING STATE");
      break;
    case TOFIRE_STATE:
      Serial.println("TOFIRE STATE");
      break;
    case FIRING_STATE:
      Serial.println("FIRING STATE");
      break;
    default:
      Serial.println("ERROR STATE");
  }
}

void debug_input() {
  Serial.print("   Fire button: ");
  Serial.println(fire_state.read() == HIGH ? "HIGH" : "LOW");
  Serial.print("   Mode switch: ");
  Serial.println(mode_state.read() == HIGH ? "HIGH" : "LOW");
  Serial.print("   Arm switch: ");
  Serial.println(arm_state.read() == HIGH ? "HIGH" : "LOW");
/*
  Serial.print("   Fire button: ");
  Serial.println(digitalRead(FIRE_PIN) == HIGH ? "HIGH" : "LOW");
  Serial.print("   Mode switch: ");
  Serial.println(digitalRead(MODE_PIN) == HIGH ? "HIGH" : "LOW");
  Serial.print("   Arm switch: ");
  Serial.println(digitalRead(ARM_PIN) == HIGH ? "HIGH" : "LOW");*/
}

void loop() {
  static unsigned long last_state_out = 0;
  arm_state.update();
  fire_state.update();
  mode_state.update();

  // indicate timing mode via LED
  if (mode_state.read() == HIGH) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  if (millis() - last_state_out > 1000) {
    debug_input();
    debug_state();
    last_state_out = millis();
  }  

  if (arm_state.rose())
    Serial.println("ARMED!");

  if (arm_state.fell()) {
    Serial.println("UNARMED");
    to_fire--;
    to_fire--; // go back two, because we'll add one when we start again
    to_fire=to_fire<0?-1:to_fire;
  }

  // If arming state is off, turn off everything
  if (arm_state.read() == LOW) {
    all_state = NO_STATE;

    for(int p = 0; p<max_sequence; p++) {
      pinMode(sequence[p], INPUT);
      digitalWrite(sequence[p], HIGH);
    }
  }

  switch(all_state) {
    case TIMING_STATE:
      timing_sm();
      break;

    case TOFIRE_STATE:
    case FIRING_STATE:
      firing_sm();
      break;

    default:
      // arming switch is marked as 
      if (arm_state.read() == HIGH && fire_state.read() == LOW) {
        Serial.println("Button pressed!");
        all_state = TIMING_STATE;
      }
  }
}
