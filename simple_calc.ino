/*Function Signatures*/

// Init Functions
void Arduino_Init();
void PPI_Init();
void LCD_Init();

// PPI Mode
void PPI_Write();
byte PPI_Read(uint8_t pin);

// Selection Functions
void Keypad();
void LCD_Screen();
void LCD_Control();

// Logic
char* Kiro_Dina_StringSplitter();
char getKey();
bool isAction(int& index);
void doAction(const int index);
void CalculateEquation();
byte isStrongOp(long& num1, long& num2, char op);
bool isOp(char c);
bool isNum(char* num);
long extractNum(char* num);
void printResult(long result);
void printError();

// Controls
void LCD_Send(uint8_t data, bool type);
void clearData();
void clearCharArray(char* arr, int sz);


// PPI
#define s0 A4
#define s1 A5
#define wr A3
#define rd A2

//LCD
#define rs 6
#define enable 7
#define LCD_COMMAND 0
#define LCD_DATA 1

// Keypad
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4

// Equation
#define equSize 16

// Define the Key-map
const char keys[KEYPAD_ROWS][KEYPAD_COLS] = {

  {'7','8','9','/'},

  {'4','5','6','*'},

  {'1','2','3','-'},

  {'c','0','=','+'}

};
byte rowPins[KEYPAD_ROWS] = { 0, 1, 2, 3 }; //Connect keypad ROW0, ROW1, ROW2 and ROW3 to these Arduino pins.
byte colPins[KEYPAD_COLS] = { 4, 5, 6, 7 }; //Connect keypad COL0, COL1 and COL2 to these Arduino pins.
  
const char opKey[6] = {'+', '-', '*', '/'};
const char actKey[2] = {'c', '='};

char equation[equSize];
byte equIndex = 0;
char pressedKey;

bool isEqualPressed = false;

// Kiro_Dina_StringSplitter Variables---------------------------
char* strokArr = NULL;
int strokSize = 0;
char strokDel = 0;
bool firstCall = false;
char* Kiro_Dina_StringSplitter
(char* arr, int sz, char del) {
  if (sz != 0 && arr != NULL) {
    strokArr = arr;
    strokSize = sz;
    firstCall = true;
  }
  else
    firstCall = false;

  char* ret = strokArr;
  for (int i = 0; i < strokSize; i++) {
    if (strokArr[i] == del) {
      strokArr[i] = '\0';
      strokArr = strokArr + i + 1;
      strokSize -= (i + 1);
      return ret;
    }
  }

  if (firstCall)
    return NULL;

  strokArr = NULL;
  strokSize = 0;

  return ret;
}

void Arduino_Init(){
  // set PortD Direction = output
  DDRD = 0xff;
  // set Controlling pins as output
  pinMode(s0, 1);
  pinMode(s1, 1);
  pinMode(rd, 1);
  pinMode(wr, 1);
}

void PPI_Init(){
  // Disable Read & Write
  digitalWrite(rd, 1);
  digitalWrite(wr, 1);
  
  // Select Control Register
  digitalWrite(s0, 1);
  digitalWrite(s1, 1);
  
  // Mode 0 three ports (port A, B, C) can work as simple input function or simple output function
  // Setting PPI Mode = 0, PortA(LCD-Controller) = output, PortB(LCD-Screen) = output, PortC(Keypad) = Input
  /*
  D7 = 1 -> Input/Output Mode
  D6,D5 = 00 -> PortA Selection Mode = 0
  D4 = 0 -> PortA is output
  D3 = 1 -> PortC Upper is input
  D2 = 0 -> PortB Selection Mode = 0
  D1 = 0 -> PortB is output
  D0 = 0 -> Portc Lower is output
  */
  PORTD = 0b10001000;
  PPI_Write();
}

void LCD_Init(){
  // Display on, Cursor off
  LCD_Send(0x0c, LCD_COMMAND);
  // Set our LCD as 2x16 screen
  LCD_Send(0x38, LCD_COMMAND);
  // Force cursor to the beginning of the first line
  LCD_Send(0x80, LCD_COMMAND);
}

void Keypad_Init(){
  Keypad();
  
  DDRD = 0x0ff;
  PORTD = 0xff;
  PPI_Write();
}

void setup() {
  Arduino_Init();
  PPI_Init();
  LCD_Init();
  Keypad_Init();
  clearCharArray(equation, equSize);
}

void clearCharArray(char *arr, int sz){
  for(int i = 0; i < sz; i++)
    arr[i] = 0;
}

void PPI_Write(){
  digitalWrite(wr, 0);
  delay(2);
  digitalWrite(wr, 1);
}

byte PPI_Read(uint8_t pin){
  digitalWrite(rd, 0);
  byte value = digitalRead(pin);
  delay(2);
  digitalWrite(rd, 1);
  
  return value;
}

void Keypad(){
  DDRD = 0x0f;
  // Select PortC/Keypad
  digitalWrite(s0, 0);
  digitalWrite(s1, 1);
}

void LCD_Screen(){
  // set PortD Direction = output
  DDRD = 0xff;
  // Select PortB
  digitalWrite(s0, 1);
  digitalWrite(s1, 0);
}

void LCD_Control(){
  // set PortD Direction = output
  DDRD = 0xff;
  // Select PortA
  digitalWrite(s0, 0);
  digitalWrite(s1, 0);
}

void loop() {    
  pressedKey = getKey(); // will wait until a key is pressed
  int i;
  if(isAction(i)){
    doAction(i);
  }
  else {
    if(isEqualPressed){
      clearData();
      isEqualPressed = false;
    }
    if (equIndex < equSize){
      LCD_Send(pressedKey, LCD_DATA);
      equation[equIndex++] = pressedKey;
    }
  }
}

char getKey(){
  // Select keypad
  Keypad();

  // Loop until a key is pressed
  while(true){
    for(int i = 0; i < KEYPAD_ROWS; i++){
      // Set row as HIGH
      digitalWrite(rowPins[i], 0);
      PPI_Write();
      
      // Find its corresponding column
      for(int j = 0; j < KEYPAD_COLS; j++){
        // if key pressed
        if(!PPI_Read(colPins[j])){
          // wait until released
          while(!PPI_Read(colPins[j]));
          
          // reSet row as LOW before returning
          digitalWrite(rowPins[i], 1);
          PPI_Write();
          
          return keys[i][j];
        }
      }
      
      // reSet row as LOW
      digitalWrite(rowPins[i], 1);
      PPI_Write();
    }
  }
}

bool isAction(int &index){
  for(index = 0; index < 2; index++){
    if(actKey[index] == pressedKey)
      return true;
  }

  return false;
}

void doAction(const int index){
  if(actKey[index] == 'c'){
    clearData();
  }
  else{
    isEqualPressed = true;
    CalculateEquation();
  }
}

void CalculateEquation(){
  // if last char isn't a number then clear due to error
  if(isOp(equation[equIndex - 1]) || equation[0] == '/' || equation[0] == '*'){
    printError();
    return;
  }

  long num1 = 0, num2 = 0;
  int code = 0;
  code = isStrongOp(num1, num2, '*');
  if(code == 1){
    printResult(num1 * num2);
  return;
  } else if (code == 2){
    printError();
    return;
  }

  // if code = 0
  code = isStrongOp(num1, num2, '/');
  if(code == 1){
    if(num2 == 0){
      printError();
      return;
    }
    printResult(num1 / num2);
    return;
  } else if (code == 2){
    printError();
    return;
  }

  // if code = 0
  char p1[equIndex], p2[equIndex];
  clearCharArray(p1, equIndex);
  clearCharArray(p2, equIndex);
  
  bool digitFound = false;
  int i;

  // get first num
  for(i = 0; i < equIndex; i++){
  if(isOp(equation[i]) && digitFound)
    break;
  else{
    p1[i] = equation[i];
    if(!isOp(equation[i]))
    digitFound = true;
  }
  }

  digitFound = false;
  // get second num
  for(int j = 0; i < equIndex; i++, j++){
  if(isOp(equation[i]) && digitFound){
    printError();
    return;
  }
  else{
    p2[j] = equation[i];
    if(!isOp(equation[i]))
    digitFound = true;
  }
  }

  //clearData();
  printResult(extractNum(p1) + extractNum(p2));
}

void printResult(long result){
  String resultStr = String(result);
  
  //For line 1: 0x80 + Column
  //For line 2: 0xc0 + Column
  LCD_Send( 0xc0 + (16 - resultStr.length()), LCD_COMMAND);
  
  for(int i = 0; i < resultStr.length(); i++)
    LCD_Send(resultStr[i], LCD_DATA);
}

// 0 -> false
// 1 -> true
// 2 -> error
byte isStrongOp(long& num1, long& num2, const char op) {
  char* p1 = Kiro_Dina_StringSplitter(equation, equIndex, op);
  if (p1 == NULL)
  return 0;

  if (!isNum(p1))
  return 2; // multiple operation are used (or more than 2 numbers)

  char* p2 = Kiro_Dina_StringSplitter(NULL, 0, op);

  if (p2 == NULL)
  return 2; // more than 1 '/' operation

  if (!isNum(p2))
  return 2; // multiple operation are used (and/or more than 2 numbers)

  if (Kiro_Dina_StringSplitter(NULL, 0, op) != NULL)
  return 2; // multiple operation are used (and/or more than 2 numbers)

  num1 = extractNum(p1);
  num2 = extractNum(p2);

  return 1;
}

bool isOp(char c){
  for(int i = 0; i < 4; i++){
    if(opKey[i] == c)
      return true;
  }

  return false;
}

bool isNum(char* num){
  char * tmp = num;
  while(*tmp != '\0'){
    if((*tmp) == '/' || (*tmp) == '*'){
      return false;
    }
    tmp++;
  }

  return true;
}

long extractNum(char* num){
  long rlst = 0;
  int cntMinus = 0;

  int i;
  for(i = 0; num[i] != 0; i++){
    if(num[i] == '-')
      cntMinus++;
    else if (num[i] != '+')
      break;
  }

  rlst = atoi(num + i);
  if(cntMinus != 0 && cntMinus % 2 != 0)
    rlst *= -1;

  return rlst;
}

void LCD_Send(uint8_t data, bool type){
  // Send data to PPI
  LCD_Screen();
  PORTD = data;
  PPI_Write();
  
  // Send data
  LCD_Control();
  // type = 0 -> command, type = 1 -> data 
  digitalWrite(rs, type);
  digitalWrite(enable, 1);
  PPI_Write();
  delay(50);
  digitalWrite(enable, LOW);
  PPI_Write();
  delay(50);
}

void clearData(){
  // Clear display command
  LCD_Send(0x01, LCD_COMMAND);
  clearCharArray(equation, equSize);
  equIndex = 0;
}

void printError(){
  clearData();
  const char error[5] = {'E', 'R', 'R', 'O', 'R'};
  for(int i = 0; i < 5; i++)
    LCD_Send(error[i], LCD_DATA);
}
