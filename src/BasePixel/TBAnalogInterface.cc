#include <stdio.h>
#include <string.h>
#include <iomanip>

#include "BasePixel/TBAnalogInterface.h"
#include "BasePixel/TBAnalogParameters.h"
#include "BasePixel/GlobalConstants.h"
#include "BasePixel/RawPacketDecoder.h"
#include "BasePixel/DigitalReadoutDecoder.h"
#include "interface/Log.h"
#include "interface/USBInterface.h"
#include "interface/Delay.h"

TBAnalogInterface::TBAnalogInterface(ConfigParameters * configParameters)
{
    Initialize(configParameters);
}


TBAnalogInterface::~TBAnalogInterface()
{
    delete cTestboard;
}


void TBAnalogInterface::Execute(SysCommand &command)
{
    int buf[2];
    int * value = &buf[0]; int * reg = &buf[1];
    int delay;
    if (command.Keyword("pon"))    {Pon();}
    else if (command.Keyword("poff"))   {Poff();}
    else if (command.Keyword("hvoff"))   {HVoff();}
    else if (command.Keyword("hvon"))   {HVon();}
    else if( command.Keyword( "usb" ) ) ShowUSB();
    else if( command.Keyword( "clear" ) ) ClearUSB();
    else if (command.Keyword("loop"))   {Intern(rctk_flag);}
    else if (command.Keyword("stop"))   {Single(0);}
    else if (command.Keyword("single")) {Single(rctk_flag);}
    else if (command.Keyword("setreg", &reg, &value)) {SetReg(*reg, *value);}
    else if (command.Keyword("ext"))    {Extern(rctk_flag);}
    else if (command.Keyword("GetRoCntEx")) {psi::LogInfo << cTestboard->GetRoCntEx() << psi::endl;}
    else if (command.Keyword("SetEmptyReadoutLength", &value)) {cTestboard->SetEmptyReadoutLength(*value);}
    else if (command.Keyword("TBMDisable")) {Tbmenable(false);}
    else if (command.Keyword("TBMChannel", &value)) {SetTBMChannel(*value);}
    else if (command.Keyword("TBMEnable")) {Tbmenable(true);}
    else if (command.Keyword("CountReadouts")) { psi::LogInfo << CountReadouts(10, 0) << " / 10" << psi::endl;}
    else if (command.Keyword("CountReadouts", &value)) {psi::LogInfo << CountReadouts(*value, 0) << " / " << *value << psi::endl;}
    else if( command.Keyword( "CountADCReadouts", &value ) ) psi::LogInfo << CountADCReadouts(*value ) << " / " << *value << psi::endl;

    else if (command.Keyword("dclear")) {DataCtrl(true,  false, false);} /* clear FIFO */
    else if (command.Keyword("dtrig"))  {DataCtrl(false, true,  false);} /* enable ADC gate once */
    else if (command.Keyword("dstart")) {DataCtrl(false, false, true);}  /* enable ADC gate continuously */
    else if (command.Keyword("dstop"))  {DataCtrl(false, false, false);} /* disable ADC gate */
    else if (command.Keyword("dena"))   {DataEnable(true);}  /* enable FIFO */
    else if (command.Keyword("ddis"))   {DataEnable(false);} /* disable FIFO */
    else if (command.Keyword("probe", &reg, &value)) {
        unsigned char port = *reg;
        unsigned char signal = *value;
        ProbeSelect(port, signal);
    }
    else if (command.Keyword("gate")) {ProbeSelect(0, PROBE_ADC_GATE);}
    else if (command.Keyword("version")) {
        char s[260];
        if (GetVersion(s, 260))
            psi::LogInfo << s << psi::endl;
        else
            psi::LogInfo << "Unable to aquire firmware version." << psi::endl;
    }
    else if (command.Keyword("scurve")) {
    int nTrig = 99;
        int dacReg = 25;
    int threshold = 20;
        int res[10000] = {0};
        int n = SCurve(nTrig, dacReg, threshold, res);
        psi::LogInfo << "SCurve:";
        for (int i = 0; i < n; i++)
            psi::LogInfo << " " << res[i];
        psi::LogInfo << psi::endl;
    }
    else if (command.Keyword("ia"))    {
        psi::LogInfo() << "[TBAnalogInterface] Analog current " << GetIA() << psi::endl;
    }
    else if (command.Keyword("id"))    {
        psi::LogInfo() << "[TBAnalogInterface] Digital current " << GetID() << psi::endl;
    }
    else if (command.Keyword("getvd"))    {
        psi::LogInfo() << "[TBAnalogInterface] Digital voltage " << GetVD() << psi::endl;
    }
    else if (command.Keyword("getva"))    {
        psi::LogInfo() << "[TBAnalogInterface] Analog voltage " << GetVA() << psi::endl;
    }
    else if (command.Keyword("res"))    {Single(0x08);}  //reset
    else if (command.Keyword("reseton"))    {ResetOn();}
    else if (command.Keyword("resetoff"))    {ResetOff();}
    else if (command.Keyword("dtlScan")) {DataTriggerLevelScan();}

    else if (strcmp(command.carg[0], "dv") == 0) {SetVD((double)*command.iarg[1] / 1000.);}
    else if (strcmp(command.carg[0], "av") == 0) {SetVA((double)*command.iarg[1] / 1000.);}
    else if (strcmp(command.carg[0], "dtl") == 0) {DataTriggerLevel(-*command.iarg[1]);}
    else if (strcmp(command.carg[0], "enableAll") == 0) SetEnableAll(*command.iarg[1]);

    else if (command.Keyword("t_res_cal", &value))
    {delay = RangeCheck(*value, 5, 255); SetTBParameter("trc", delay);}
    else if (command.Keyword("trc", &value))
    {delay = RangeCheck(*value, 5, 255); SetTBParameter("trc", delay);}
    else if (command.Keyword("t_cal_cal", &value))
    {delay = RangeCheck(*value, 5, 255); SetTBParameter("tcc", delay);}
    else if (command.Keyword("tcc", &value))
    {delay = RangeCheck(*value, 5, 255); SetTBParameter("tcc", delay);}
    else if (command.Keyword("t_cal_trig", &value))
    {delay = RangeCheck(*value, 5, 255); SetTBParameter("tct", delay);}
    else if (command.Keyword("tct", &value))
    {delay = RangeCheck(*value, 5, 255); SetTBParameter("tct", delay);}
    else if (command.Keyword("t_trg_tok", &value))
    {delay = RangeCheck(*value, 5, 255); SetTBParameter("ttk", delay);}
    else if (command.Keyword("ttk", &value))
    {delay = RangeCheck(*value, 5, 255); SetTBParameter("ttk", delay);}
    else if (command.Keyword("t_rep", &value))
    {delay = RangeCheck(*value, 1, 120); SetTBParameter("trep", delay);}
    else if (command.Keyword("trep", &value))
    {delay = RangeCheck(*value, 1, 120); SetTBParameter("trep", delay);}
    else if (command.Keyword("calrep", &value))
    {delay = RangeCheck(*value, 1, 120); SetTBParameter("cc", delay);}
    else if (command.Keyword("cc", &value))
    {delay = RangeCheck(*value, 1, 120); SetTBParameter("cc", delay);}
    else if (command.Keyword("seq", &value))
    {rctk_flag = RangeCheck(*value, 0, 15);}
    else if (command.Keyword("ctr", &value))
    {delay = RangeCheck(*value, 1, 120); SetTBParameter("ctr", delay);}
    else
    {
        if (!tbParameters->Execute(command))
            psi::LogInfo() << "[TBAnalogInterface] Unknown testboard command: "
                           << command.carg[0] << psi::endl;
    }

}


// == General functions ================================================


void  TBAnalogInterface::Pon()
{
    cTestboard->Pon();
    cTestboard->Flush();
}


void TBAnalogInterface::Poff()
{
    cTestboard->Poff();
    cTestboard->Flush();
}


void TBAnalogInterface::Set(int reg, int value)
{
    cTestboard->Set(reg, value);
}


void TBAnalogInterface::SetReg(int reg, int value)
{
    cTestboard->SetReg(reg, value);
}


void TBAnalogInterface::Single(int mask)
{
    cTestboard->Single(mask);
}


void TBAnalogInterface::Intern(int mask)
{
    cTestboard->Intern(mask);
}


void TBAnalogInterface::Extern(int mask)
{
    cTestboard->Extern(mask);
}


int TBAnalogInterface::GetRoCnt()
{
    return cTestboard->GetRoCntEx();
}

//----------------------------------------------------------------------
void TBAnalogInterface::ShowUSB() // USB parameters
{
  cTestboard->ShowUSB(); // in psi46_tb.h
}

//----------------------------------------------------------------------
void TBAnalogInterface::ClearUSB() // reset USB buffer
{
  cTestboard->Clear(); // in psi46_tb.h
}

//------------------------------------------------------------------------------
void TBAnalogInterface::Initialize(ConfigParameters * configParameters)
{

    tbParameters = (TBParameters *)new TBAnalogParameters(this);

    signalCounter = 0;
    readPosition = 0;
    writePosition = 0;
    triggerSource = 0;

    cTestboard = new CTestboard();
    if (!cTestboard->Open(configParameters->testboardName))
        return;
    fIsPresent = 1;

    cTestboard->Welcome();

    char s[260];
    GetVersion(s, 260);
    psi::LogInfo() << "---- TestBoard Version" << s << psi::endl;


    if (configParameters->tbmEnable) {
        tbmenable = 1;
        SetTriggerMode(TRIGGER_MODULE2);
    }
    else {
        tbmenable = 0;
        SetTriggerMode(TRIGGER_ROC);
    }

    if (configParameters->tbmEmulator) {
        cTestboard->TBMEmulatorOn();
        psi::LogInfo() << "TBM emulator on" << psi::endl;
    }
    else {
        cTestboard->TBMEmulatorOff();
        psi::LogInfo() << "TBM emulator off" << psi::endl;
    }

    Pon();
    I2cAddr(0);
    rctk_flag = 15;

    SetTBMChannel(configParameters->tbmChannel);
    Tbmenable(configParameters->tbmEnable);

    SetIA(configParameters->ia);
    SetID(configParameters->id);
    SetVA(configParameters->va);
    SetVD(configParameters->vd);

    SetEmptyReadoutLength(configParameters->emptyReadoutLength);
    SetEmptyReadoutLengthADC(configParameters->emptyReadoutLengthADC);
    SetEmptyReadoutLengthADCDual(configParameters->emptyReadoutLengthADCDual);

    if (configParameters->hvOn) HVon();
    DataTriggerLevel(configParameters->dataTriggerLevel);

    cTestboard->SetHubID(configParameters->hubId);
    cTestboard->SetNRocs(configParameters->nRocs);
    cTestboard->SetEnableAll(0);

    DataEnable(true);
    cTestboard->ResetOn(); // send hard reset to connected modules / TBMs
    cTestboard->Flush();
    gDelay->Mdelay(100);
    cTestboard->ResetOff();
    cTestboard->Flush();

    ReadTBParameterFile(configParameters->GetTbParametersFileName());    //only after power on
}


void TBAnalogInterface::Clear()
{
    cTestboard->Clear();
}


int TBAnalogInterface::Startup(int port)
{
    return 1;
}


void TBAnalogInterface::Cleanup()
{
    cTestboard->Close();
}


int TBAnalogInterface::Present()
{
    return 1;
}


void TBAnalogInterface::I2cAddr(int id)
{
    cTestboard->I2cAddr(id);
}


void TBAnalogInterface::SetTriggerMode(unsigned short mode)
{
    cTestboard->SetTriggerMode(mode);
}


void TBAnalogInterface::SetEmptyReadoutLength(int length)
{
    emptyReadoutLength = length;
    cTestboard->SetEmptyReadoutLength(length);
}


int TBAnalogInterface::GetEmptyReadoutLength()
{
    return emptyReadoutLength;
}


void TBAnalogInterface::SetEmptyReadoutLengthADC(int length)
{
    emptyReadoutLengthADC = length;
    cTestboard->SetEmptyReadoutLengthADC(length);
}


void TBAnalogInterface::SetEmptyReadoutLengthADCDual(int length)
{
    emptyReadoutLengthADCDual = length;
}


int TBAnalogInterface::GetEmptyReadoutLengthADC()
{
    return emptyReadoutLengthADC;
}


int TBAnalogInterface::GetEmptyReadoutLengthADCDual()
{
    return emptyReadoutLengthADCDual;
}


void TBAnalogInterface::SetEnableAll(int value)
{
    cTestboard->SetEnableAll(value);
}


unsigned short TBAnalogInterface::GetModRoCnt(unsigned short index)
{
    return cTestboard->GetModRoCnt(index);
}


// == TBM functions ======================================================

void TBAnalogInterface::Tbmenable(int on)
{
    TBMpresent = on;
    cTestboard->tbm_Enable(on);
    SetReg41();
}


void TBAnalogInterface::ModAddr(int hub)
{
    cTestboard->mod_Addr(hub);
}


void TBAnalogInterface::TbmAddr(int hub, int port)
{
    cTestboard->tbm_Addr(hub, port);
}


int TBAnalogInterface::TbmWrite(const int hubAddr, const int addr, const int value)
{
    if (!cTestboard->TBMPresent()) return -1;
    cTestboard->TbmWrite(hubAddr, addr, value);
    return 0;
}


int TBAnalogInterface::Tbm1write(const int hubAddr, const int registerAddress, const int value)
{
    if (!cTestboard->TBMPresent()) return -1;
    cTestboard->Tbm1Write(hubAddr, registerAddress, value);
    return 0;
}


int TBAnalogInterface::Tbm2write(const int hubAddr, const int registerAddress, const int value)
{
    if (!cTestboard->TBMPresent()) return -1;
    cTestboard->Tbm2Write(hubAddr, registerAddress, value);
    return 0;
}


bool TBAnalogInterface::GetTBMReg(int reg, int &value)
{
    unsigned char v, r = (unsigned char)reg;
    bool result = cTestboard->tbm_Get(r, v);
    value = (int)v;
    return result;
}


// == ROC functions ======================================================


void TBAnalogInterface::SetChip(int chipId, int hubId, int portId, int aoutChipPosition)
{
    cTestboard->tbm_Addr(hubId, portId);
    cTestboard->roc_I2cAddr(chipId);
    cTestboard->SetAoutChipPosition(aoutChipPosition);
}

void TBAnalogInterface::RocClrCal()
{
    cTestboard->roc_ClrCal();
}


void TBAnalogInterface::RocSetDAC(int reg, int value)
{
    cTestboard->roc_SetDAC(reg, value);
}


void TBAnalogInterface::RocPixTrim(int col, int row, int value)
{
    cTestboard->roc_Pix_Trim(col, row, value);
}


void TBAnalogInterface::RocPixMask(int col, int row)
{
    cTestboard->roc_Pix_Mask(col, row);
}


void TBAnalogInterface::RocPixCal(int col, int row, int sensorcal)
{
    cTestboard->roc_Pix_Cal(col, row, sensorcal);
}


void TBAnalogInterface::RocColEnable(int col, int on)
{
    cTestboard->roc_Col_Enable(col, on);
}


void TBAnalogInterface::Flush()
{
    cTestboard->Flush();
}


void TBAnalogInterface::SetClock(int n)
{
    cTestboard->SetClock(n);
    Flush();
}


// == Analog functions =================================================


void TBAnalogInterface::DataTriggerLevel(int level)
{
    cTestboard->DataTriggerLevel(TBMChannel, level);
    cTestboard->SetDTL(level);
}


void TBAnalogInterface::DataCtrl(bool clear, bool trigger, bool cont)
{
    cTestboard->DataCtrl(TBMChannel, clear, trigger, cont);
}


void TBAnalogInterface::DataEnable(bool on)
{
    cTestboard->DataEnable(on);
}



bool TBAnalogInterface::DataRead(short buffer[], unsigned short buffersize, unsigned short &wordsread)
{
    return cTestboard->DataRead(TBMChannel, buffer, buffersize, wordsread);
}


void TBAnalogInterface::SetDelay(int signal, int ns)
{
    cTestboard->SetDelay(signal, ns);
    Flush();
}

void TBAnalogInterface::SetClockStretch(unsigned char src, unsigned short delay, unsigned short width)
{
    cTestboard->SetClockStretch(src, delay, width);
    Flush();
}


void TBAnalogInterface::CDelay(unsigned int clocks)
{
    cTestboard->cDelay(clocks);
}


bool TBAnalogInterface::SendRoCnt()
{
    //works only for trigger mode MODULE1
    //  if (signalCounter % 100 == 0) Log::Current()->printf("counter %i\n", signalCounter);
    //  Log::Current()->printf("counter %i\n", signalCounter);
    if (signalCounter == 30000) ReadBackData();
    signalCounter++;
    return cTestboard->SendRoCntEx();
}


int TBAnalogInterface::RecvRoCnt()
{
    //works only for trigger mode MODULE1
    //  Log::Current()->printf("counter %i\n", signalCounter);
    //  if (signalCounter % 100 == 0) Log::Current()->printf("counter %i\n", signalCounter);
    if (signalCounter == 0  && readPosition == writePosition)  //buffer empty and nothing to read
    {
        psi::LogInfo() << "[TBAnalogInterface] Error: no signal to read from testboard."
                       << psi::endl;
        return -1;
    }
    else if (readPosition == writePosition) {   //buffer is empty
        signalCounter--;
        return cTestboard->RecvRoCntEx();
    }
    else {
        int data = dataBuffer[readPosition];   //buffer not empty
        readPosition++;
        if (readPosition == bufferSize) readPosition = 0;
        return data;
    }
}


//------------------------------------------------------------------------------
void TBAnalogInterface::SingleCal()
{
    Single(RES | CAL | TRG | TOK);
    CDelay(500); //CDelay(100) is too short
}


int TBAnalogInterface::CountReadouts(int count, int chipId)
{
    return cTestboard->CountReadouts(count, chipId);
}


void TBAnalogInterface::SendCal(int nTrig)
{
    for (int i = 0; i < nTrig; i++)
    {
        SingleCal();
        SendRoCnt();
    }
}


int TBAnalogInterface::CountADCReadouts(int count)
{
    unsigned short counter;
    short data[FIFOSIZE];

    int n = 0;
    for (int i = 0; i < count; i++)
    {
        DataCtrl(false, true); // no clear, trigger
        Single(RES | CAL | TRG | TOK);
        CDelay(100);
        Flush();
        DataRead(data, FIFOSIZE, counter);
        // n += ((int)counter - 56) / 6; // Module
		n += ( (int)counter - 19 ) / 6; // Single ROC, with TBM emu
    }
    return n;
}


bool TBAnalogInterface::ADCData(short buffer[], unsigned short &wordsread)
{
    ADCRead(buffer, wordsread);
    return true;
}

unsigned short TBAnalogInterface::ADC()
{
    /* sends a single trigger and displays the raw readout on the console */
    unsigned short count;
    short data[FIFOSIZE];
    bool is_analog = IsAnalog();
    if (is_analog)
        ADCRead(data, count);
    else
        ADCRead_digital(data, count);

    psi::LogInfo << "[TBAnalogInterface] Count: " << count << (is_analog ? "" : " bits") << psi::endl;

    psi::LogInfo << "[TBAnalogInterface] Data: ";
    for (unsigned int n = 0; n < count; n++) {
        if (is_analog)
	 	{
    		psi::LogInfo << " " << setw(4) << data[n];
	    	if( n ==  7 ) psi::LogInfo << " :"; // after TBM header
			if( n == 10 ) psi::LogInfo << " :"; // after UB, B, lastDAC
    		if( n > 15 && n < count - 7 && (n-11)%6 == 5 ) psi::LogInfo << " :"; // after each pixel
		} else
            psi::LogInfo << ((n % 8 == 0) ? "|" : "") << ((data[n / 16] & (1 << (16 - n % 16 - 1))) ? 1 : 0);
    }

    psi::LogInfo << psi::endl;
    return count;
}

unsigned short TBAnalogInterface::ADC(int nbsize)
{

    unsigned short count;
    short data[FIFOSIZE];
    ADCRead(data, count);
    // probe with the scope the Gate and the comp output signal
    //cTestboard->ProbeSelect(0,PROBE_ADC_COMP);
    //cTestboard->ProbeSelect(1,PROBE_ADC_GATE);
    //  cout<<"&&&&&&& TBAnalogInterface::ADCData "<<endl;
    //  cout<<"start testing the reset"<<endl;
    //  cTestboard->ResetOn();
    //  cTestboard->mDelay(1000);
    //  cTestboard->ResetOff();
    //  cout<<"reset On and off done!"<<endl;
    //  cTestboard->Flush();

    if (nbsize > 0)
    {
        // run adc with fix trigger mode
        if (IsAnalog())
            cTestboard->SetReg(41, 32);
        else
            cTestboard->SetReg(41, 33);
        cTestboard->SetTriggerMode(TRIGGER_FIXED);
        cTestboard->DataBlockSize(200);
        cTestboard->DataCtrl(0, false, true, false);
        cTestboard->Single(RES | CAL | TRG);
        cTestboard->mDelay(100);
        cTestboard->DataRead(0, data, FIFOSIZE, count);
        cTestboard->mDelay(100);
        cTestboard->Intern(RES | CAL | TRG);
        cTestboard->Flush();
        //  cTestboard->Welcome();
    }

    psi::LogDebug() << "[TBAnalogInterface] Count " << count << psi::endl;
    cout << "[TBAnalogInterface] Count " << count << endl;

    //  for (unsigned int n = 0; n < count; n++) data[n] &= 0xf000;

    psi::LogDebug << "[TBAnalogInterface] Data: ";
    cout << "[TBAnalogInterface] Data: ";
    for (unsigned int n = 0; n < count; n++)
    {
        psi::LogDebug << " " << data[n];
        cout << " " << data[n];
    }
    psi::LogDebug << psi::endl;
    cout << endl;


    if (tbmenable)SetTriggerMode(TRIGGER_MODULE2);
    else SetTriggerMode(TRIGGER_ROC);

    return count;
}




// -- sends n calibrate signals and gives back the resulting ADC readout
void TBAnalogInterface::SendADCTrigs(int nTrig)
{
    for (int i = 0; i < nTrig; i++)
    {
        DataCtrl(false, true); // no clear, trigger
        Single(RES | CAL | TRG | TOK);
        CDelay(500);
    }
}


int TBAnalogInterface::LastDAC(int nTriggers, int chipId)
{
    int numRepetitions = 0;

    unsigned short count = 0;
    short data[FIFOSIZE];
    while (count == 0 && numRepetitions < 100) {
        ADCRead(data, count, nTriggers);

        //cout << "ADC = { ";
        //for ( int i = 0; i < count; i++ ){
        //  cout << data[i] << " ";
        //}
        //cout << "} " << endl;

        numRepetitions++;
    }

    if (numRepetitions >= 100) {
        cerr << "Error in <TBAnalogInterface::LastDAC>: cannot find ADC signal !" << endl;
        return 0;
    }

    return data[10 + chipId * 3];
}


void TBAnalogInterface::SendADCTrigsNoReset(int nTrig)
{
    for (int i = 0; i < nTrig; i++)
    {
        DataCtrl(false, true); // no clear, trigger
        Single(CAL | TRG);
        CDelay(500);
    }
}


bool TBAnalogInterface::GetADC(short buffer[], unsigned short buffersize, unsigned short &wordsread, int nTrig, int startBuffer[], int &nReadouts)
{
    RawPacketDecoder * gDecoder = RawPacketDecoder::Singleton();
    nReadouts = 0;

    while (!DataRead(buffer, buffersize, wordsread))
    {
        Clear();
        cout << "usb cleared" << endl;
        return false;
    }


    if (wordsread > 0) {
        for (int pos = 0; pos < (wordsread - 2); pos++)
        {
            if (gDecoder->isUltraBlackTBM(buffer[pos]) && gDecoder->isUltraBlackTBM(buffer[pos + 1]) && gDecoder->isUltraBlackTBM(buffer[pos + 2]))
            {
                if (nReadouts < nTrig) startBuffer[nReadouts] = pos;
                nReadouts++;
            }
        }
    }

    return (nReadouts <= nTrig);
}



bool TBAnalogInterface::DataTriggerLevelScan()
{
    unsigned short count;
    bool result = false;
    for (int delay = 0; delay < 2000; delay = delay + 50)
    {
        psi::LogDebug() << "[TBAnalogInterface] dtl: " << delay
                        << " -------------------------------------" << psi::endl;

        DataTriggerLevel(-delay);
        Flush();
        count = ADC();
        if (count == emptyReadoutLengthADC) result = true;
    }
    return result;
}




void TBAnalogInterface::SetVA(double V)
{
    cTestboard->SetVA(V);
}


void TBAnalogInterface::SetIA(double A)
{
    cTestboard->SetIA(A);
}

void TBAnalogInterface::SetVD(double V)
{
    cTestboard->SetVD(V);
}


void TBAnalogInterface::SetID(double A)
{
    return cTestboard->SetID(A);
}


double TBAnalogInterface::GetVA()
{
    return cTestboard->GetVA();
}


double TBAnalogInterface::GetIA()
{
    return cTestboard->GetIA();
}


double TBAnalogInterface::GetVD()
{
    return cTestboard->GetVD();
}


double TBAnalogInterface::GetID()
{
    return cTestboard->GetID();
}


void TBAnalogInterface::HVon()
{
    cTestboard->HVon();
}


void TBAnalogInterface::HVoff()
{
    cTestboard->HVoff();
}


void TBAnalogInterface::ResetOn()
{
    cTestboard->ResetOn();
}


void TBAnalogInterface::ResetOff()
{
    cTestboard->ResetOff();
}


void TBAnalogInterface::SetTBMChannel(int channel)
{
    TBMChannel = channel;
    cTestboard->SetTbmChannel(channel);
    SetReg41();
}


int TBAnalogInterface::GetTBMChannel()
{
    return TBMChannel;
}

bool TBAnalogInterface::IsAnalog()
{
    return (TBMChannel == 0);
}


// ----------------------------------------------------------------------
bool TBAnalogInterface::Mem_ReadOut(FILE * f, unsigned int addr, unsigned int size) {

	// Can be tuned to be faster. Was: unsigned short BLOCKSIZE = 32767;
    unsigned short BLOCKSIZE = 50000;
    unsigned char buffer[BLOCKSIZE];
    for (int i = 0; i < BLOCKSIZE; i++) buffer[i] = 0;

    Flush();
    Clear();
    unsigned int bound = static_cast<unsigned int>(2. * size / BLOCKSIZE);
    unsigned int start = addr;

	cout << "r/o of " << 2.*size
	     << " bytes with blocksize " << BLOCKSIZE
    	 << " starting from memory address " << addr
	     << endl;
    for (unsigned int j = 0; j < bound; ++j) {
        cTestboard->MemRead(start, BLOCKSIZE, buffer);
        start += BLOCKSIZE;
        fwrite(buffer, BLOCKSIZE, 1, f);
    	cout << "read " << (j+1)*BLOCKSIZE << " of " << 2*size << endl;
    }

    unsigned short rest = (addr + 2 * size - start);
    cTestboard->MemRead(start, rest, buffer);
    fwrite(buffer, rest, 1, f);
    Clear();
}



void TBAnalogInterface::SetReg41()
{
    int value(0);
    value = TBMChannel;
    if (TBMpresent) value += 2;
    if (triggerSource) value += 16;
    else value += 32;

    SetReg(41, value);
}


void TBAnalogInterface::StartDataTaking()
{
    DataCtrl(false, false, true); // go
    SetReg(43, 2);
    SetReg(41, 0x2A);
    Flush();
}


void TBAnalogInterface::StopDataTaking()
{
    SetReg(41, 0x22);
    DataCtrl(false, false, false); // stop
    Flush();
}


// == buffer functions ===========================================================================

void TBAnalogInterface::ReadBackData()
{
    Flush();
    //  Log::Current()->printf("reading back data\n");
    for (int i = 0; i < signalCounter; i++)
    {
        dataBuffer[writePosition] = cTestboard->RecvRoCntEx();
        writePosition++;
        if (writePosition == bufferSize) {writePosition = 0;}
        if (writePosition == readPosition)
        {
            psi::LogInfo() << "[TBAnalogInterface] Error: Signalbuffer full in "
                           << "TBAnalogInterface ! Data loss possible !!!"
                           << psi::endl;
            return;
        }
        //      Log::Current()->printf("wr %i\n", writePosition);
    }
    // //   Log::Current()->printf("reading back data done\n");
    signalCounter = 0;
}


int TBAnalogInterface::AoutLevel(int position, int nTriggers)
{
    return cTestboard->AoutLevel(position, nTriggers);
}


void TBAnalogInterface::DoubleColumnADCData(int doubleColumn, short data[], int readoutStop[])
{
    cTestboard->DoubleColumnADCData(doubleColumn, data, readoutStop);
}


int TBAnalogInterface::ChipThreshold(int start, int step, int thrLevel, int nTrig, int dacReg, int xtalk, int cals, int trim[], int res[])
{
    DataEnable(false);
    int n =  cTestboard->ChipThreshold(start, step, thrLevel, nTrig, dacReg, xtalk, cals, trim, res);
    DataEnable(true);
    return n;
}


int TBAnalogInterface::AoutLevelChip(int position, int nTriggers, int trims[], int res[])
{
    return cTestboard->AoutLevelChip(position, nTriggers, trims, res);
}


int TBAnalogInterface::AoutLevelPartOfChip(int position, int nTriggers, int trims[], int res[], bool pxlFlags[])
{
    return cTestboard->AoutLevelPartOfChip(position, nTriggers, trims, res, pxlFlags);
}


int TBAnalogInterface::ChipEfficiency(int nTriggers, int trim[], double res[])
{
    DataEnable(false);
    int n = cTestboard->ChipEfficiency(nTriggers, trim, res);
    DataEnable(true);
    return n;
}


int TBAnalogInterface::MaskTest(short nTriggers, short res[])
{
    DataEnable(false);
    int n = cTestboard->MaskTest(nTriggers, res);
    DataEnable(true);
    return n;
}



int TBAnalogInterface::PixelThreshold(int col, int row, int start, int step, int thrLevel, int nTrig, int dacReg, int xtalk, int cals, int trim)
{
    DataEnable(false);
    int n = cTestboard->PixelThreshold(col, row, start, step, thrLevel, nTrig, dacReg, xtalk, cals, trim);
    DataEnable(true);
    return n;
}


int TBAnalogInterface::SCurve(int nTrig, int dacReg, int threshold, int res[])
{
    DataEnable(false);
    int n = cTestboard->SCurve(nTrig, dacReg, threshold, res);
    DataEnable(true);
    return n;
}


int TBAnalogInterface::SCurveColumn(int column, int nTrig, int dacReg, int thr[], int trims[], int chipId[], int res[])
{
    DataEnable(false);
    int n = cTestboard->SCurveColumn(column, nTrig, dacReg, thr, trims, chipId, res);
    DataEnable(true);
    return n;
}


void TBAnalogInterface::ADCRead(short buffer[], unsigned short &wordsread, short nTrig)
{
    /* send nTrig calibrates and record the raw analog readout into buffer */
    cTestboard->ADCRead(buffer, wordsread, nTrig);
}


void TBAnalogInterface::ADCRead_digital(short buffer[], unsigned short &bitsread, short nTrig)
{
    /* send nTrig calibrates and record the raw digital readout into buffer */
    if (!buffer)
        return;

    /* Read the data as you would read the analog data */
    unsigned short wordsread;
    cTestboard->ADCRead(buffer, wordsread, nTrig);

	DecodedReadoutModule * drm = new DecodedReadoutModule;

	int retval = decode_digital_readout( drm, buffer, wordsread, 1, 0 );
	bitsread = 4 * wordsread;

	psi::LogInfo << "[TBAnalogInterface] Count: " << bitsread << " bits" << psi::endl;
	psi::LogInfo << "[TBAnalogInterface] Data: ";

    /* compactify: bit shift the data to remove the leading 12 bits in each word */
    /* data: 1000|0000|0000|XXXX (only XXXX is significant data) */
    int nibble = 0;
    for (int i = 0; i < wordsread; i++) {
        int word = i / 4;
        buffer[word] &= ~(0xf << ((4 - nibble - 1) * 4));
        buffer[word] |= (buffer[i] & 0xf) << ((4 - nibble - 1) * 4);
        nibble = (nibble + 1) % 4;
    }


	for( unsigned int n = 0; n < bitsread; n++ ) {
	  psi::LogInfo << ( (n % 8 == 0) ? "|" : "") << ( (buffer[n / 16] & (1 << (16 - n % 16 - 1) ) ) ? 1 : 0 );
	}
 	psi::LogInfo << psi::endl;

	if( retval >= 0 ) { // Successful decoding:
	    int nhits = drm->roc[0].numPixelHits;
	    cout << nhits << " pixel hits" << endl;
		for( int ii = 0; ii < nhits; ++ii ) {
	    	// Record the pulse height and move to the next block of data
	    	int ph = drm->roc[0].pixelHit[ii].analogPulseHeight;
	    	int col = drm->roc[0].pixelHit[ii].columnROC;
	    	int row = drm->roc[0].pixelHit[ii].rowROC;
	    	cout << "hit " << setw(4) << ii+1;
	    	cout << ": col " << setw(2) << col;
		    cout << ", row " << setw(2) << row;
		    cout << ", PH " << setw(3) << ph;
		    cout << endl;
		}
	}
	else {
		cout << "digital decoder error" << endl;
	}
}


void TBAnalogInterface::DacDac(int dac1, int dacRange1, int dac2, int dacRange2, int nTrig, int result[])
{
    DataEnable(false);
    cTestboard->DacDac(dac1, dacRange1, dac2, dacRange2, nTrig, result);
    DataEnable(true);
}


void TBAnalogInterface::PHDac(int dac, int dacRange, int nTrig, int position, short result[])
{
    cTestboard->PHDac(dac, dacRange, nTrig, position, result);
}

int TBAnalogInterface::PH(int col, int row, int nTrig)
{
    return cTestboard->PH(col, row, nTrig);
}

bool TBAnalogInterface::test_pixel_address(int col, int row)
{
    return cTestboard->test_pixel_address(col, row);
}

void TBAnalogInterface::AddressLevels(int position, int result[])
{
    cTestboard->AddressLevels(position, result);
}


void TBAnalogInterface::TBMAddressLevels(int result[])
{
    cTestboard->TBMAddressLevels(result);
}


void TBAnalogInterface::TrimAboveNoise(short nTrigs, short thr, short mode, short result[])
{
    DataEnable(false);
    cTestboard->TrimAboveNoise(nTrigs, thr, mode, result);
    DataEnable(true);
}

// --------------------------------------------------------

void TBAnalogInterface::ProbeSelect(unsigned char port, unsigned char signal) {
    cTestboard->ProbeSelect(port, signal);
}


int TBAnalogInterface::demo(short x)
{
    return cTestboard->demo(x);
}


void TBAnalogInterface::ScanAdac(unsigned short chip, unsigned char dac,
                                 unsigned char min, unsigned char max, char step,
                                 unsigned char rep, unsigned int usDelay, unsigned char res[])
{
    DataEnable(false);
    cTestboard->ScanAdac(chip, dac, min, max, step, rep, usDelay, res);
    DataEnable(true);
}

void TBAnalogInterface::CdVc(unsigned short chip, unsigned char wbcmin, unsigned char wbcmax, unsigned char vcalstep,
                             unsigned char cdinit, unsigned short &lres, unsigned short res[])
{
    DataEnable(false);
    cTestboard->CdVc(chip, wbcmin, wbcmax, vcalstep, cdinit, lres, res);
    DataEnable(true);
}

char TBAnalogInterface::CountAllReadouts(int nTrig, int counts[], int amplitudes[])
{
    return cTestboard->CountAllReadouts(nTrig, counts, amplitudes);
}
bool TBAnalogInterface::GetVersion(char * s, unsigned int n)
{
    return cTestboard->GetVersion(s, n);
}
