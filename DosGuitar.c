
//FSM.h

struct State
{
  State(void (*on_enter)(), void (*on_state)(), void (*on_exit)());
  void (*on_enter)();
  void (*on_state)();
  void (*on_exit)();
};


class Fsm
{
public:
  Fsm(State* initial_state);
  ~Fsm();

  void add_transition(State* state_from, State* state_to, int event,
                      void (*on_transition)());

  void add_timed_transition(State* state_from, State* state_to,
                            unsigned long interval, void (*on_transition)());

  void check_timed_transitions();

  void trigger(int event);
  void run_machine();

private:
  struct Transition
  {
    State* state_from;
    State* state_to;
    int event;
    void (*on_transition)();

  };
  struct TimedTransition
  {
    Transition transition;
    unsigned long start;
    unsigned long interval;
  };

  static Transition create_transition(State* state_from, State* state_to,
                                      int event, void (*on_transition)());

  void make_transition(Transition* transition);

private:
  State* m_current_state;
  Transition* m_transitions;
  int m_num_transitions;

  TimedTransition* m_timed_transitions;
  int m_num_timed_transitions;
  bool m_initialized;
};
//end FSM.h

//FSM.cpp

State::State(void (*on_enter)(), void (*on_state)(), void (*on_exit)())
: on_enter(on_enter),
  on_state(on_state),
  on_exit(on_exit)
{
}


Fsm::Fsm(State* initial_state)
: m_current_state(initial_state),
  m_transitions(NULL),
  m_num_transitions(0),
  m_num_timed_transitions(0),
  m_initialized(false)
{
}

Fsm::~Fsm()
{
  free(m_transitions);
  free(m_timed_transitions);
  m_transitions = NULL;
  m_timed_transitions = NULL;
}


void Fsm::add_transition(State* state_from, State* state_to, int event,
                         void (*on_transition)())
{
  if (state_from == NULL || state_to == NULL)
    return;

  Transition transition = Fsm::create_transition(state_from, state_to, event,
                                               on_transition);
  m_transitions = (Transition*) realloc(m_transitions, (m_num_transitions + 1)
                                                       * sizeof(Transition));
  m_transitions[m_num_transitions] = transition;
  m_num_transitions++;
}


void Fsm::add_timed_transition(State* state_from, State* state_to,
                               unsigned long interval, void (*on_transition)())
{
  if (state_from == NULL || state_to == NULL)
    return;

  Transition transition = Fsm::create_transition(state_from, state_to, 0,
                                                 on_transition);

  TimedTransition timed_transition;
  timed_transition.transition = transition;
  timed_transition.start = 0;
  timed_transition.interval = interval;

  m_timed_transitions = (TimedTransition*) realloc(
      m_timed_transitions, (m_num_timed_transitions + 1) * sizeof(TimedTransition));
  m_timed_transitions[m_num_timed_transitions] = timed_transition;
  m_num_timed_transitions++;
}


Fsm::Transition Fsm::create_transition(State* state_from, State* state_to,
                                       int event, void (*on_transition)())
{
  Transition t;
  t.state_from = state_from;
  t.state_to = state_to;
  t.event = event;
  t.on_transition = on_transition;

  return t;
}

void Fsm::trigger(int event)
{
  if (m_initialized)
  {
    // Find the transition with the current state and given event.
    for (int i = 0; i < m_num_transitions; ++i)
    {
      if (m_transitions[i].state_from == m_current_state &&
          m_transitions[i].event == event)
      {
        Fsm::make_transition(&(m_transitions[i]));
        return;
      }
    }
  }
}

void Fsm::check_timed_transitions()
{
  for (int i = 0; i < m_num_timed_transitions; ++i)
  {
    TimedTransition* transition = &m_timed_transitions[i];
    if (transition->transition.state_from == m_current_state)
    {
      if (transition->start == 0)
      {
        transition->start = millis();
      }
      else{
        unsigned long now = millis();
        if (now - transition->start >= transition->interval)
        {
          Fsm::make_transition(&(transition->transition));
          transition->start = 0;
        }
      }
    }
  }
}

void Fsm::run_machine()
{
  // first run must exec first state "on_enter"
  if (!m_initialized)
  {
    m_initialized = true;
    if (m_current_state->on_enter != NULL)
      m_current_state->on_enter();
  }
  
  if (m_current_state->on_state != NULL)
    m_current_state->on_state();
    
  Fsm::check_timed_transitions();
}

void Fsm::make_transition(Transition* transition)
{
 
  // Execute the handlers in the correct order.
  if (transition->state_from->on_exit != NULL)
    transition->state_from->on_exit();

  if (transition->on_transition != NULL)
    transition->on_transition();

  if (transition->state_to->on_enter != NULL)
    transition->state_to->on_enter();
  
  m_current_state = transition->state_to;

  //Initialice all timed transitions from m_current_state
  unsigned long now = millis();
  for (int i = 0; i < m_num_timed_transitions; ++i)
  {
    TimedTransition* ttransition = &m_timed_transitions[i];
    if (ttransition->transition.state_from == m_current_state)
      ttransition->start = now;
  }

}
//end FSM.cpp

//Triggers
#define piezo_no_sinal 0    
#define piezo_sinal 1
#define piezos_musical 2
#define finish_sound 3

#define NOTE_G5 784
#define NOTE_D5 587
#define NOTE_B5 988
#define NOTE_E5 659
#define NOTE_A5 880

const int buzzer = 10;

int piezo[] = {
    A0, A1, A2, A3, A4,
};

int piezoState = piezo_no_sinal; 

void no_sound();
void piezo_check();
void piezo_identify();
void piezo_identify_when();
void play_song();
void after_play_song();
void play();

//States

State state_no_sound(&no_sound, &piezo_check, NULL);
State state_piezo_verify(&piezo_identify, &piezo_identify_when, NULL);
State state_play_music(&play_song, &after_play_song, NULL);
State state_play_note(&play, &after_play_song, NULL);


Fsm soundFsm(&state_no_sound);

int marry_melody[] = {
  NOTE_G5, NOTE_G5, NOTE_G5, NOTE_D5,
  NOTE_E5, NOTE_E5, NOTE_D5,
  NOTE_B5, NOTE_B5, NOTE_B5, NOTE_A5, NOTE_A5,
  NOTE_G5, NOTE_D5,
  NOTE_G5, NOTE_G5, NOTE_G5, NOTE_D5,
  NOTE_E5, NOTE_E5, NOTE_D5,
  NOTE_B5, NOTE_B5, NOTE_A5, NOTE_A5,
  NOTE_G5
};

int noteDurations[] = {
  4,4,4,4,
  4,4,2,
  4,4,4,4,
  2,4,4,
  4,4,4,4,
  4,4,2,
  4,4,4,4,
  2,
};

int scale_1[] = {
  262,294,330,349,392
};

int scale_2[] = {
  523,587,659,698,784
};

int scale_3[] = {
  1047,1175,1319,1397,1568
};

int piezo_Reading[] = {
    0,0,0,0,0
};

void no_sound(){
  digitalWrite(buzzer, LOW);
};

void piezo_check(){
  for(int i = 0; i < 5 ; i++){
      piezo_Reading[i] = analogRead(piezo[i]);
  }
  
  if(piezo_Reading[0] > 50 || piezo_Reading[1] > 50 || piezo_Reading[2] > 50 || piezo_Reading[3] > 50 || piezo_Reading[4] > 50){
      piezoState = piezo_sinal;
      soundFsm.trigger(piezo_sinal);
  }

  if(piezo_Reading[0] == 0 && piezo_Reading[1] == 0 && piezo_Reading[2] == 0 && piezo_Reading[3] == 0 && piezo_Reading[4] == 0){
      piezoState = piezo_no_sinal;
      soundFsm.trigger(piezo_no_sinal);
  }
}

void piezo_identify(){
    if(piezo_Reading[0] > 800 && piezo_Reading[4] > 800){
        piezoState = piezos_musical;
    }
    else {
        piezoState = piezo_sinal;
    }
}

void piezo_identify_when(){
    
    if(piezoState == piezos_musical){
        soundFsm.trigger(piezos_musical);
    }
    else {
        soundFsm.trigger(piezo_sinal);
    }
}


void play_song(){
  for (int thisNote = 0; thisNote < 26; thisNote++) {
    int noteDuration = 1000/noteDurations[thisNote];
    tone(buzzer, marry_melody[thisNote],noteDuration);

    int pauseBetweenNotes = noteDuration* 1.30;
    delay(pauseBetweenNotes);
    noTone(buzzer);
  }
}

void after_play_song(){
    piezoState = finish_sound;
    soundFsm.trigger(finish_sound);
}

int scale(int frequencyInHertz, int n_piezo){
  if(frequencyInHertz >= 50 && frequencyInHertz <500){
    return scale_1[n_piezo];
  }
  else if(frequencyInHertz >= 501 && frequencyInHertz <800){
    return scale_2[n_piezo];
  }
  else{
    return scale_3[n_piezo];
  }
}

void play(){
  for(int i = 0; i < 5 ; i++){
      if(piezo_Reading[i] > 50){
       int frequencyInHertz = scale(piezo_Reading[i], i);
        long delayAmount = (long)(1000000/frequencyInHertz);
        
        long cycles = frequencyInHertz * 100/1000;
        
        if(cycles < 0){
            cycles = cycles * (-1);
        }
        for(long i=0; i < cycles; i++){
            digitalWrite(buzzer, HIGH);
            delayMicroseconds(delayAmount);
            digitalWrite(buzzer,LOW);
            delayMicroseconds(delayAmount);
        }

      }
  }    
 
}

void addTransitions(){
  soundFsm.add_transition(&state_no_sound, &state_piezo_verify, piezo_sinal, NULL);
  soundFsm.add_transition(&state_piezo_verify, &state_no_sound, piezo_no_sinal, NULL);
  soundFsm.add_transition(&state_piezo_verify, &state_play_music, piezos_musical, NULL);
  soundFsm.add_transition(&state_play_music, &state_no_sound, finish_sound, NULL);
  soundFsm.add_transition(&state_piezo_verify, &state_play_note,  piezo_sinal, NULL);
  soundFsm.add_transition(&state_play_note, &state_no_sound,  finish_sound, NULL);
}

void setup(){
  pinMode(buzzer, OUTPUT);
  addTransitions();
  Serial.begin(9600);
}

void loop(){

Serial.print("piezo_01: "); Serial.print(piezo_Reading[0]); Serial.print("\t");
Serial.print("piezo_02: "); Serial.print(piezo_Reading[1]); Serial.print("\t");
Serial.print("piezo_03: "); Serial.print(piezo_Reading[2]); Serial.print("\t");
Serial.print("piezo_04: "); Serial.print(piezo_Reading[3]); Serial.print("\t");
Serial.print("piezo_05: "); Serial.print(piezo_Reading[4]); Serial.print("\t");
Serial.println("\t");

soundFsm.run_machine();
}