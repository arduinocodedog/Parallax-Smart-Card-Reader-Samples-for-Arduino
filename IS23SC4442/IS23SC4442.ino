// THIS SOFTWARE IS PROVIDED TO YOU "AS IS," AND WE MAKE NO EXPRESS OR IMPLIED WARRANTIES WHATSOEVER 
// WITH RESPECT TO ITS FUNCTIONALITY, OPERABILITY, OR USE, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED 
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR INFRINGEMENT. WE EXPRESSLY 
// DISCLAIM ANY LIABILITY WHATSOEVER FOR ANY DIRECT, INDIRECT, CONSEQUENTIAL, INCIDENTAL OR SPECIAL 
// DAMAGES, INCLUDING, WITHOUT LIMITATION, LOST REVENUES, LOST PROFITS, LOSSES RESULTING FROM BUSINESS 
// INTERRUPTION OR LOSS OF DATA, REGARDLESS OF THE FORM OF ACTION OR LEGAL THEORY UNDER WHICH THE 
// LIABILITY MAY BE ASSERTED, EVEN IF ADVISED OF THE POSSIBILITY OR LIKELIHOOD OF SUCH DAMAGES. 

// Use Card 32321

// This Sample Application Performs the following (All Output goes to the Serial Monitor):
//
// 1 - Resets Card
// 2 - Authenticates Password (FF FF FF).  
//     Note: Being that I locked my first card ... and then found out even the PBasic 2.5 and Propeller Demos would automatically lock after 3 card inserts
//           I have taken pains to make sure that it shouldn't happen again ... 
//           but if you need that type of security ... 
//           you may want to look at the compare function and make some changes.
//           But for testing ... locking up the card (so you can't write values) shouldn't be possible with this code (but no guarantees).
// 3 - Writes characters 0 through 255 to memory location 0 through 255 (although location 0 will stay at A2)
// 4 - Displays a memory dump
// 5 - Clears all memory locations (sets all locations to 0 although again, location 0 will stay at A2)
// 6 - Displays another memory dump.
//
// Components required:
//
// An Arduino or compatible should work ... 
//    I tested with an Arduino UNO SMD and an Arduino Mega 2560 R3 ... both worked.
// A Parallax Smart Card Reader.
// A IS23SC4442 (32321) Smart Card.
//

#define IO   A4    // Input/Output Pin
#define CLK  A5    // Clock Pin

#define RST  2     // Reset Pin
#define CD   3     // Card Detect Pin

#define ReadMain           0x30  // Read main memory
#define ReadProtection     0x34  // Read protection bit
#define ReadSecurity       0x31  // Read PSC
#define UpdateMain         0x38  // Update main memory
#define UpdateSecurity     0x39  // Update PSC
#define UpdateProtection   0x3C  // Update Protection bits
#define CompareData        0x33  // Compare Verification Data

bool CardDetected = 0;   // Last Card Detect Event
int delayamt = 50;      // Default Delay Amount (in Microseconds)

void setup() 
{
  Serial.begin(9600);
  
  Input(CD);  // Initialize Card Detect Pin for Input
  
  Serial.println("Note: 32321 Card Expected");

  CardDetected = IsCardDetected();
  
  if (!CardDetected)
    Serial.println("Please insert a Card...");
  else
    Serial.println("Card was already inserted at startup...Please remove and re-insert card.");
}

void loop() 
{
  bool cd = IsCardDetected();
  
  if (cd != CardDetected)
  {
    if (cd) 
    { 
      Serial.println("Card Inserted!");
      Reset();
      Authenticate();
      Serial.println("Update Main...");
      for (int i = 0; i <= 255; i++)
        Update_Main((uint8_t) i, (uint8_t) i);
      Serial.println(" Done.");
      Read_Main();
      Serial.println("Clear Main...");
      for (int i = 0; i <= 255; i++)
        Update_Main((uint8_t) i, (uint8_t) 0);
      Serial.println(" Done.");
      Read_Main();
    }
    else
      Serial.println("Card Removed!");
    CardDetected = cd;
  }
  
  delay(1000);
}

bool IsCardDetected()
{
  return Get(CD);
}

void Reset()
{
  Serial.println("Reset.");
  Output(CLK);
  Input(IO);
  Output(RST);
  Set_Low(RST);
  Set_Low(CLK);
  Set_High(RST);
  delayMicroseconds(delayamt); // A little extra wait time for this
  Set_High(CLK);
  Set_Low(CLK);
  delayMicroseconds(delayamt); // A little extra wait time for this
  Set_Low(RST);
  long address = 0;
  for (int index = 0; index < 4; index++)
  {
    Receive_Byte();
  }
}

void Authenticate()
{
  Serial.println("Authenticating...");
  
  uint8_t errCounter = Compare();
  switch (errCounter)
  {
    case 0x00:  // Hopefully we'll NEVER see this ... because once it's locked it can't be unlocked as far as I know
      Serial.println(" Card is locked.");
      break;
    case 0x01:
      Serial.println(" Invalid PSC, one try remaining.");
      break;
    case 0x03:
      Serial.println(" Invalid PSC, two tries remaining.");
      break;
    case 0x07:
      Serial.println(" PSC verified, you may now write data to the card.");
      break;
    default:
      Serial.println(" Unabled to Authenticate.");
      break;
  }
  
  Serial.println(" Done.");
}

uint8_t Compare()
{
  uint8_t Data = 0x03;  // Just make sure we never lock the Card
  
  uint8_t Password1 = 0xFF;  // 1st byte of PSC
  uint8_t Password2 = 0xFF;  // 2nd byte of PSC 
  uint8_t Password3 = 0xFF;  // 3rd byte of PSC
  
  uint8_t errCounter = Read_Security();
  
  errCounter = errCounter & 0x07;

  switch (errCounter)
  {
    case 0x07:
      Data = 0x03;
      break;
    case 0x03:
      Data = 0x03;  // was 0x01 ... only one less and we'll lock the card
      break;
    case 0x01:
      Data = 0x03;  // was 0x00 ... which will lock the Card
      break;
    case 0x00:
      Data = 0x03;
      break;
  }
  
  Send_Command(UpdateSecurity, 0x00, Data);
  
  Send_Command(CompareData, 0x01, Password1);
  
  Send_Command(CompareData, 0x02, Password2);

  Send_Command(CompareData, 0x03, Password3);
  
  Send_Command(UpdateSecurity, 0x00, 0x07);   // Reset Counter to it's highest value (so we don't ever lock the Card)
 
  return errCounter;
}

void Read_Main()
{
  Serial.println("Read Main...");
  
  uint8_t bstr[16];
  int bcnt = 0;
 
  Send_Command(ReadMain, 0x00, 0x00);
  
  Serial.println("                            [ D A T A ]");
  Serial.print(" * |");
  for (int i = 0; i < 16; i++)
  {
    Serial.print("  ");
    Serial.print(i, HEX);
  }
  Serial.print("    characters");
  Serial.println("");
  Serial.println(" --+-----------------------------------------------------------------");
  for (int index = 0; index <= 255; index++)
  {
    if ((index % 16) == 0)
    {
      if (index != 0)
        Serial.println("");
      Serial.print(" ");
      Serial.print(index / 16, HEX);
      Serial.print(" |");
    }
    uint8_t Temp = Receive_Byte();
    bstr[bcnt++] = Temp;
    Serial.print(" ");
    if (Temp < 0x10) // add a leading zero if < 10 hex.
      Serial.print("0");
    Serial.print(Temp, HEX);
    if  ((index % 16) == 15)
    {
        Serial.print(" ");
        for (int b = 0; b < 16; b++)
        {
          if (bstr[b] < 0x20 || bstr[b] > 0x7f)
            Serial.print('.');
          else
            Serial.print((char)bstr[b]);
        }
        bcnt = 0;
    }
  }
  Serial.println("");
  Serial.println(" --------------------------------------------------------------------");
  Serial.println(" Done.");  
}

void Update_Main(uint8_t Address, uint8_t Data)
{
  Send_Command(UpdateMain, Address, Data);
}

uint8_t Read_Security()
{
  Serial.println(" Read Security...");
  
  Send_Command(ReadSecurity, 0x00, 0x00);
  uint8_t errCounter = Receive_Byte();
 
  Send_Command(ReadSecurity, 0x01, 0x00);
  uint8_t PSW1 = Receive_Byte();
  
  Send_Command(ReadSecurity, 0x02, 0x00);
  uint8_t PSW2 = Receive_Byte();
  
  Send_Command(ReadSecurity, 0x03, 0x00);
  uint8_t PSW3 = Receive_Byte();
  
  Serial.print("  errCounter = ");
  Serial.print((uint8_t)errCounter, HEX);
  Serial.print(", PSW1 = ");
  Serial.print((uint8_t)PSW1, HEX);
  Serial.print(", PSW2 = ");
  Serial.print((uint8_t)PSW2, HEX);
  Serial.print(", PSW3 = ");
  Serial.print((uint8_t)PSW3, HEX);
  Serial.println("");
  Serial.println("  Done.");
  
  return errCounter;
}

void Send_Command(uint8_t Command, uint8_t Address, uint8_t Data) 
{
  Output(CLK);
  Set_High(CLK);
  Output(IO);
  Set_Low(IO);
  _SendToCard(Command);
  _SendToCard(Address);
  _SendToCard(Data);
  Set_Low(CLK);
  Set_Low(IO);
  Set_High(CLK);
  Set_High(IO);
  
  if (Command == UpdateMain || Command == UpdateProtection || Command == CompareData || Command == UpdateSecurity)
    Processing();
}

// I tried to get shiftOut to work, but it didn't.
void _SendToCard(uint8_t cmd)
{ 
    uint8_t command = cmd;
    bool temp = 0;
    
    for (int i = 0; i < 8; i++)
    {
      temp = command & 0x01;
      command = command >> 1;
      Set_Low(CLK);
      if (temp != 0)
        Set_High(IO);
      else
        Set_Low(IO);
      Set_High(CLK);
    }
}

uint8_t Receive_Byte()
{
  return _ReceiveFromCard();
}

// I tried to get shiftIn to work, but it didn't.
uint8_t _ReceiveFromCard()
{
  uint8_t data = 0;
  bool bit = 0;
  
  Output(CLK);
  Input(IO);
  Set_High(CLK);
  
  for (int i = 0; i < 8; i++)
  {
      data = data >> 1;
      Set_Low(CLK);   
      Set_High(CLK);
      bit = Get(IO);
      if (bit) 
      {
        data += 0x80;
      }
  }
  
  return data;
}

void Processing()
{
  Input(IO);
  Set_Low(CLK);
  
  while (Get(IO) == 0)
  {
    Set_High(CLK);
    Set_Low(CLK);
  }
}

void Set_Low(int Pin)
{
  digitalWrite(Pin, LOW);
  delayMicroseconds(delayamt);
}

void Set_High(int Pin)
{
  digitalWrite(Pin, HIGH);
  delayMicroseconds(delayamt);
}

bool Get(int Pin)
{
  return digitalRead(Pin);
}

void Input(int Pin)
{
  pinMode(Pin, INPUT);
}

void Output(int Pin)
{
  pinMode(Pin, OUTPUT);
}

