// THIS SOFTWARE IS PROVIDED TO YOU "AS IS," AND WE MAKE NO EXPRESS OR IMPLIED WARRANTIES WHATSOEVER 
// WITH RESPECT TO ITS FUNCTIONALITY, OPERABILITY, OR USE, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED 
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR INFRINGEMENT. WE EXPRESSLY 
// DISCLAIM ANY LIABILITY WHATSOEVER FOR ANY DIRECT, INDIRECT, CONSEQUENTIAL, INCIDENTAL OR SPECIAL 
// DAMAGES, INCLUDING, WITHOUT LIMITATION, LOST REVENUES, LOST PROFITS, LOSSES RESULTING FROM BUSINESS 
// INTERRUPTION OR LOSS OF DATA, REGARDLESS OF THE FORM OF ACTION OR LEGAL THEORY UNDER WHICH THE 
// LIABILITY MAY BE ASSERTED, EVEN IF ADVISED OF THE POSSIBILITY OR LIKELIHOOD OF SUCH DAMAGES.

// Use Card 32322

// This Sample Application Performs the following (All output goes to the Serial Monitor):
//
// 1 - Writes same character to each memory location 0 through 255 dependent upon page #
//     i.e. Writes '1' for Page 1, '2' for Page 2, '3' for Page 3 ... all the way to page 8.
// 2 - Displays a memory dump of all 8 pages
// 3 - Clears all memory locations of all pages (sets all locations to 0)
// 4 - Displays another memory dump of all 8 pages
//
// Components required:
//
// An Arduino or compatible should work ... 
//    I tested with an Arduino UNO SMD and an Arduino Mega 2560 R3 ... both worked.
// A Parallax Smart Card Reader.
// A IS24C16A (32322) Smart Card.
//

#define IO   A4    // Input/Output Pin
#define CLK  A5    // Clock Pin

#define RST  2     // Reset Pin
#define CD   3     // Card Detect Pin

#define ACK  0        // ACK
#define NAK  1        // NAK
#define EEPROM 0xA0   // EEPROM Address

bool CardDetected = 0;   // Last Card Detect Event
int delayamt = 50;       // Default Delay Amount (in Microseconds)

void setup() 
{
  Serial.begin(9600);
  
  Input(CD);  // Initialize the Card Detect Pin for Input
  
  Serial.println("Note: 32322 Card Expected");
  
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
      
      Initialize();
      
      for (uint8_t page = 0; page < 8; page++)
      {
        if ( devicePresent(EEPROM + (page << 1)) )
          Write_Memory(page);
        else
          Serial.println("Device not present.");  
      }
      
      for (uint8_t page = 0; page < 8; page++)
      {
        if ( devicePresent(EEPROM + (page << 1)) )
          Read_Memory(page);
        else
          Serial.println("Device not present.");  
      }

      for (uint8_t page = 0; page < 8; page++)
      {
        if ( devicePresent(EEPROM + (page << 1)) )
          Clear_Memory(page);
        else
          Serial.println("Device not present.");  
      }
      
      for (uint8_t page = 0; page < 8; page++)
      {
        if ( devicePresent(EEPROM + (page << 1)) )
          Read_Memory(page);
        else
          Serial.println("Device not present.");  
      }
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

void Clear_Memory(uint8_t page)
{
  Serial.print("Clearing Memory...Page ");
  Serial.println(page + 1);
  
  for (int i = 0; i <= 255; i++)
  {
    writeLocation(page, EEPROM + (page << 1), (uint8_t) i, 0x00);
  }
  
  Serial.println(" Done.");
}

void Write_Memory(uint8_t page)
{
  Serial.print("Writing Memory...Page ");
  Serial.println(page + 1);
  
  for (int i = 0; i <= 255; i++)
  {
    writeLocation(page, EEPROM + (page << 1), (uint8_t)  i, (uint8_t) (page + '1'));
  }
  
  Serial.println(" Done.");
}

void Read_Memory(uint8_t page)
{
  Serial.print("Read Main...Page ");
  Serial.println(page + 1);
  
  uint8_t bstr[16];
  int bcnt = 0;
 
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
    uint8_t Temp = readLocation(EEPROM + (page << 1), (uint8_t) index);
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

void Initialize()
{
  Serial.println("Initialize.");
  
  Output(CLK);
  Input(IO);
  Set_High(CLK);
  
  for (int i = 0; i < 9; i++)
  {
    Set_Low(CLK);
    Set_High(CLK);
    
    if (Get(IO) != 0)
      break;
  }
}

void Start()
{
  Output(IO);
  Set_High(IO);
  Output(CLK);
  Set_High(CLK);
  Set_Low(IO);
  Set_Low(CLK);
}

void Stop()
{
  Output(CLK);
  Set_Low(CLK);
  Output(IO);
  Set_Low(IO);
  Set_High(CLK);
  Set_High(IO); 
  Input(CLK);
  Input(IO);
}

bool devicePresent(uint8_t deviceAddress)
{
  Start();
  bool ackbit = Write(deviceAddress | 0);
  Stop();
  
  if (ackbit == ACK)
    return true;
  else
    return false;
}

bool Write(uint8_t data)
{
  bool ackbit = 0;
  
  Output(IO);
  Output(CLK);
  
  shiftOut(IO, CLK, MSBFIRST, data);
  
  Input(IO);
  
  do
  {
    Set_High(CLK);
    ackbit = Get(IO);
    Set_Low(CLK);
  } while (ackbit != ACK);

  Output(IO);
  Set_Low(IO);
  
  return ackbit;
}

uint8_t Read(bool ackbit)
{
  uint8_t data = 0;
  
  Input(IO);
  Output(CLK);
  
  data = shiftIn(IO, CLK, MSBFIRST);
  
  if (ackbit == ACK)
    Set_Low(IO);
  else
    Set_High(IO); 
  
  Output(IO);
  Set_High(CLK);
  Set_Low(CLK);
  Set_Low(IO);
  
  return data;
}

void writeLocation(uint8_t page, uint8_t device_address, uint8_t reg, uint8_t value)
{
  delayamt = 400;
  Start();
  delayamt = 50;
  Write(device_address);
  Write(reg);
  Write(value);
  delayamt = 400;
  Stop();
  Process();
  while ( !devicePresent( EEPROM + (page << 1) ) );
  delayamt = 50;
}

uint8_t readLocation(uint8_t device_address, uint8_t reg)
{
  uint8_t value = 0;
  
  Start();
  Write(device_address);
  Write(reg);
  
  Start();
  Write(device_address + 1);
  value = Read(NAK);
  Stop();

  return value;
}

void Process()
{
  Output(CLK);
  Input(IO);
  
  if (Get(IO) != 0)
  {
    Set_Low(CLK);
    Set_High(CLK);
  }
  
  Input(CLK);
  Input(IO);
  
  delayMicroseconds(1000);
}
 
bool Get(int Pin)
{
  return digitalRead(Pin);
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

void Input(int Pin)
{
  pinMode(Pin, INPUT);
}

void Output(int Pin)
{
  pinMode(Pin, OUTPUT);
}

