/*!------------------------------------------------
*
*  Demo programm for EIB7
*
*  \file    trigger_io.c
*  \author  DR.JOHANNES HEIDENHAIN GmbH
*  \date    03.11.2009
*  \version $Revision: 1.1 $
* 
*  \brief   sample for external trigger I/O
*  
*  Content:
*  Sample programm for the trigger outputs of
*  the EIB. The program configures one axis
*  of the EIB for incremental encoders and
*  enables the soft realtime mode. The axis will
*  be triggered by the internal timer trigger. 
*  Additionally these trigger pulses will be
*  forwarded to the trigger output 1.
*
-----------------------------------------------------*/


#include <eib7.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef Linux
#include <signal.h>
#endif

#ifdef _WIN32
#define POS_SPEC "Trg-Cntr: %05u, Timestamp: %010lu, Status: 0x%04X, Pos: %010I64d"
#else
#define POS_SPEC "Trg-Cntr: %05u, Timestamp: %010lu, Status: 0x%04X, Pos: %010lld"
#endif


/* definitions */
#define EIB_TCP_TIMEOUT   5000   /* timeout for TCP connection in ms      */
#define NUM_OF_AXIS       4      /* number of axes of the EIB             */
#define NUM_OF_IO         4      /* number of inputs and outputs          */
#define MAX_SRT_DATA      200    /* maximum size of soft realtime data    */
#define MAX_TEXT_LEN      200    /* maximum size of console input string  */
#define TIMESTAMP_PERIOD  1000   /* Timestamp Period = 1 ms = 1000us      */
#define TRIGGER_PERIOD    500000 /* Trigger Period = 0.5 sec = 500000us   */


/* struct for soft realtime mode data */
struct EncData
{
   ENCODER_POSITION position;             /* position value              */
   unsigned short status;                 /* status word                 */
   unsigned short TriggerCounter;         /* trigger counter value       */
   unsigned long Timestamp;               /* timestamp                   */
   ENCODER_POSITION ref[2];               /* reference position values   */
};


/* function declarations */
void CheckError(EIB7_ERR error);


/* global variable for console handler to stop on user request */
static int stop = 0;

#ifdef _WIN32

/* handler for console to catch user inputs */
BOOL CtrlHandler( DWORD fdwCtrlType )
{
   if(fdwCtrlType == CTRL_C_EVENT)
   {
      stop = TRUE;
      return TRUE;
   }

   return FALSE;
}
#endif

#ifdef Linux

/* handler for console to catch user inputs */
void CtrlHandler(int sig)
{
   if(sig==SIGINT)
     stop = 1;
}
#endif



/* Softrealtime demo program
   This program demonstrates the soft realtime mode of the EIB.
   The program initialises one axis of the EIB and reads the
   position data. The status word, the position value, the
   timestamp and the trigger counter are displayed.
   */
int main(int argc, char **argv)
{
   EIB7_HANDLE eib;                       /* EIB handle                  */
   unsigned long ip;                      /* IP address of EIB           */
   unsigned long num;                     /* number of encoder axes      */
   unsigned long TimerTicks;              /* timer ticks per us          */
   unsigned long TimerPeriod;             /* timer trigger period        */
   unsigned long TimestampTicks;          /* ticks per us (timestamp)    */
   unsigned long TimestampPeriod;         /* period of timestamp counter */
   EIB7_AXIS axis[NUM_OF_AXIS];           /* axes array                  */
   EIB7_IO input[NUM_OF_IO];              /* IO array (input)            */
   EIB7_IO output[NUM_OF_IO];             /* IO array (output)           */
   EIB7_DataPacketSection packet[5];      /* Data packet configuration   */
   char fw_version[20];                   /* firmware version string     */
   char hostname[MAX_TEXT_LEN];           /* hostname string             */
   int enc_axis;                          /* actual axis index           */
   unsigned char udp_data[MAX_SRT_DATA];  /* buffer for udp data packet  */
   struct EncData EncoderData;            /* data to display             */
   unsigned long entries;                 /* entries read from FIFO      */
   void *field;                           /* pointer to data field       */
   unsigned long sz;                      /* size of data field          */
   EIB7_ERR error;                        /* error code                  */
   unsigned long ilen, olen;              /* number of inputs/outputs    */
   int i;
   ENCODER_POSITION* pos_value;


/* register console handler for program termination on user request */
#ifdef _WIN32
   SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE );
#endif
#ifdef Linux
   signal(SIGINT, CtrlHandler);
   signal(SIGTERM, CtrlHandler);
#endif


   /* ask for parameters */
   printf("Please enter IP address or hostname: ");
   scanf("%s", hostname);
   printf("Please select encoder axis to display (1..4): ");
   scanf("%i", &i);
   if(i<1 || i>4)
   {
      printf("wrong axis number\n");
      exit(1);
   }
   enc_axis=i-1;

   /* open connection to EIB */
   CheckError(EIB7GetHostIP(hostname, &ip));
   CheckError(EIB7Open(ip, &eib, EIB_TCP_TIMEOUT, fw_version, sizeof(fw_version)));
   printf("\nEIB firmware version: %s\n\n", fw_version);

   /* read axes array */
   CheckError(EIB7GetAxis(eib, axis, NUM_OF_AXIS, &num));

   for(i=0; i<NUM_OF_AXIS; i++)
   {
      /* init axis: 1 Vpp */
      printf("Initializing encoder %d for 1 Vpp\n", i+1);
      CheckError(EIB7InitAxis(axis[i],
                 EIB7_IT_Incremental,
                 EIB7_EC_Linear,
                 EIB7_RM_None,         /* reference marks not used */
                 0,                    /* reference marks not used */
                 0,                    /* reference marks not used */
                 EIB7_HS_None,
                 EIB7_LS_None,
                 EIB7_CS_CompActive,   /* signal compensation on   */
                 EIB7_BW_High,         /* signal bandwidth: high   */
                 EIB7_CLK_Default,     /* not used for incremental interface */
                 EIB7_RT_Long,         /* not used for incremental interface */
                 EIB7_CT_Long          /* not used for incremental interface */
               ));

      CheckError(EIB7AxisTriggerSource(axis[i], EIB7_AT_TrgTimer));
   }
      
   TimestampPeriod = TIMESTAMP_PERIOD;
   CheckError(EIB7GetTimestampTicks(eib, &TimestampTicks));
   TimestampPeriod *= TimestampTicks;
   CheckError(EIB7SetTimestampPeriod(eib, TimestampPeriod));
   CheckError(EIB7SetTimestamp(axis[enc_axis], EIB7_MD_Enable));
   CheckError(EIB7StartRef(axis[enc_axis], EIB7_RP_RefPos2));

   /* configure data packet */
   CheckError(EIB7AddDataPacketSection(packet, 0, EIB7_DR_Global, EIB7_PDF_TriggerCounter));
   CheckError(EIB7AddDataPacketSection(packet, 1, EIB7_DR_Encoder1, EIB7_PDF_StatusWord | EIB7_PDF_PositionData | EIB7_PDF_Timestamp | EIB7_PDF_ReferencePos));
   CheckError(EIB7AddDataPacketSection(packet, 2, EIB7_DR_Encoder2, EIB7_PDF_StatusWord | EIB7_PDF_PositionData | EIB7_PDF_Timestamp | EIB7_PDF_ReferencePos));
   CheckError(EIB7AddDataPacketSection(packet, 3, EIB7_DR_Encoder3, EIB7_PDF_StatusWord | EIB7_PDF_PositionData | EIB7_PDF_Timestamp | EIB7_PDF_ReferencePos));
   CheckError(EIB7AddDataPacketSection(packet, 4, EIB7_DR_Encoder4, EIB7_PDF_StatusWord | EIB7_PDF_PositionData | EIB7_PDF_Timestamp | EIB7_PDF_ReferencePos));
   CheckError(EIB7ConfigDataPacket(eib, packet, 5));

   /* enable trigger output */
   /* get IO handles for configuration */
   CheckError(EIB7GetIO(eib, input, NUM_OF_IO, &ilen, output, NUM_OF_IO, &olen));
   /* enable trigger input, disable termination resistor */
   CheckError(EIB7InitOutput(output[0], EIB7_IOM_Trigger, EIB7_MD_Enable));
   CheckError(EIB7OutputTriggerSource(output[0], EIB7_OT_TrgTimer));

   /* set timer trigger period */
   CheckError(EIB7GetTimerTriggerTicks(eib, &TimerTicks));
   TimerPeriod = TRIGGER_PERIOD;
   TimerPeriod *= TimerTicks;
   CheckError(EIB7SetTimerTriggerPeriod(eib, TimerPeriod));

   CheckError(EIB7MasterTriggerSource(eib, EIB7_AT_TrgTimer));
   
   /* enable SoftRealtime mode */
   CheckError(EIB7SelectMode(eib, EIB7_OM_SoftRealtime));

   /* enable timer trigger */
   CheckError(EIB7GlobalTriggerEnable(eib, EIB7_MD_Enable, EIB7_TS_TrgTimer));

   printf("\n\npress Ctrl-C to stop\n\n");

   while(!stop)
   {
      /* run till the user press Ctrl-C */

      /* read data packet from FIFO */
      error = EIB7ReadFIFOData(eib, udp_data, 1, &entries, 0);
      if(error==EIB7_FIFOOverflow)
      {
         printf("FIFO Overflow error detected, clearing FIFO.\n");
         EIB7ClearFIFO(eib);
      }

      if(entries > 0)
      {
         /* read trigger counter from data packet */
         CheckError(EIB7GetDataFieldPtr(eib, udp_data, EIB7_DR_Global, 
                    EIB7_PDF_TriggerCounter, &field, &sz));
         EncoderData.TriggerCounter = *(unsigned short *)field;

         /* read timestamp from data packet */
         CheckError(EIB7GetDataFieldPtr(eib, udp_data, enc_axis + 1,
                    EIB7_PDF_Timestamp, &field, &sz));
         EncoderData.Timestamp = *(unsigned long *)field;

         /* read position value from data packet */
         CheckError(EIB7GetDataFieldPtr(eib, udp_data, enc_axis + 1,
                    EIB7_PDF_PositionData, &field, &sz));
         EncoderData.position = *(ENCODER_POSITION *)field;

         /* read status word from data packet */
         CheckError(EIB7GetDataFieldPtr(eib, udp_data, enc_axis + 1,
                    EIB7_PDF_StatusWord, &field, &sz));
         EncoderData.status = *(unsigned short *)field;

         /* read reference positions from data packet */
         CheckError(EIB7GetDataFieldPtr(eib, udp_data, enc_axis + 1,
                    EIB7_PDF_ReferencePos, &field, &sz));
         pos_value = (ENCODER_POSITION *)field;
         EncoderData.ref[0] = pos_value[0];
         EncoderData.ref[1] = pos_value[1];

         /* print status word and position value */
         printf(POS_SPEC, EncoderData.TriggerCounter, EncoderData.Timestamp,
                          EncoderData.status, EncoderData.position);
         printf("\n");
      }

#ifdef _WIN32          /* wait for 50 ms to minmize processor load */
      Sleep(50);
#else
      usleep(50000);
#endif

   } /* end of loop */

   /* disable trigger */
   CheckError(EIB7GlobalTriggerEnable(eib, EIB7_MD_Disable, EIB7_TS_All));

   /* disable SoftRealtime mode */
   CheckError(EIB7SelectMode(eib, EIB7_OM_Polling));

   /* close connection to EIB */
   EIB7Close(eib);

   printf("\nStopped on user request\n");

   exit(1);
}


/* check error code
   This function prints the error code an a brief description to the standard error console.
   The program will be terminated afterwards.
   */
void CheckError(EIB7_ERR error)
{
   if(error != EIB7_NoError)
   {
      char mnemonic[32];
      char message[256];

      EIB7GetErrorInfo(error, mnemonic, 32, message, 256);

      fprintf(stderr, "\nError %08x (%s): %s\n", error, mnemonic, message);
      exit(0);
   }
}
