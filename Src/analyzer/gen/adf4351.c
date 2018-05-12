#include "stm32746g_discovery.h"
#include "adf4351.h"
#include <math.h>
#include "si5351.h"
#include "custom_spi2.h"


#define ADF5451_PFD_MAX                 32.0e6
#define ADF4351_RFOUT_MAX               4400.0e6
#define ADF4351_RFOUTMIN                34.375e6
#define ADF4351_REFINMAX                250.0e6


/** @brief  Union type for the structure of Register0 in ADF4351
 */
typedef union
{
    struct
    {
        uint32_t ControlBits        :3;       ///< bit:  0.. 2     CONTROL BITS
        uint32_t FracVal            :12;      ///< bit:  3..14     12-BIT FRACTIONAL VALUE (FRAC)
        uint32_t IntVal             :16;      ///< bit: 15..30     16- BIT INTEGER VALUE (INT)
        uint32_t _reserved_0        :1;       ///< bit: 31         RESERVED
    } b;
    uint32_t w;
} ADF4351_Reg0_t;



/** @brief  Union type for the structure of Register1 in ADF4351
 */
typedef union
{
    struct
    {
        uint32_t ControlBits        :3;          ///< bit:  0.. 2     CONTROL BITS
        uint32_t ModVal             :12;         ///< bit:  3..14     12-BIT MODULUS VALUE (MOD)
        uint32_t PhaseVal           :12;         ///< bit: 15..26     12-BIT PHASE VALUE (PHASE)
        uint32_t Prescaler          :1;          ///< bit:  27        PRESCALER
        uint32_t PhaseAdjust        :1;          ///< bit:  28        PHASE ADJUST
        uint32_t _reserved_0        :3;          ///< bit: 29..31     RESERVED
    } b;
    uint32_t w;
} ADF4351_Reg1_t;


/** @brief  Union type for the structure of Register2 in ADF4351
 */
typedef union
{
    struct
    {
        uint32_t ControlBits        :3;         ///< bit:  0.. 2     CONTROL BITS
        uint32_t CounterReset       :1;         ///< bit:  3         Counter Reset
        uint32_t CPTristate         :1;         ///< bit:  4         Charge Pump Three-State
        uint32_t PowerDown          :1;         ///< bit:  5         Power-Down
        uint32_t PhasePolarity      :1;         ///< bit:  6         Phase Detector Polarity
        uint32_t LockPrecision      :1;         ///< bit:  7         Lock Detect Precision
        uint32_t LockFunction       :1;         ///< bit:  8         Lock Detect Function
        uint32_t CPCurrent          :4;         ///< bit:  9..12     Charge Pump Current Setting
        uint32_t DoubleBuffer       :1;         ///< bit:  13        Double Buffer
        uint32_t RCountVal          :10;        ///< bit: 14..23     10-Bit R Counter
        uint32_t RDiv2              :1;         ///< bit: 24         Double Buffer
        uint32_t RMul2              :1;         ///< bit: 25         Double Buffer
        uint32_t MuxOut             :3;         ///< bit: 26..28     MUXOUT
        uint32_t LowNoiseSpur       :2;         ///< bit: 29..30     Low Noise and Low Spur Modes
        uint32_t _reserved_0        :1;         ///< bit: 31         RESERVED
    } b;
    uint32_t w;
} ADF4351_Reg2_t;



/** @brief  Union type for the structure of Register3 in ADF4351
 */
typedef union
{
    struct
    {
        uint32_t ControlBits      :3;          ///< bit:  0.. 2  CONTROL BITS
        uint32_t ClkDivVal        :12;         ///< bit:  3..14  12-Bit Clock Divider Value
        uint32_t ClkDivMod        :2;          ///< bit:  15..16 Clock Divider Mode
        uint32_t _reserved_0      :1;          ///< bit:  17     RESERVED
        uint32_t CsrEn            :1;          ///< bit:  18     CSR Enable
        uint32_t _reserved_1      :2;          ///< bit:  19..20 RESERVED
        uint32_t ChargeCh         :1;          ///< bit:  21     Charge Cancelation
        uint32_t AntibacklashW    :1;          ///< bit:  22     Antibacklash Pulse Width
        uint32_t BandSelMode      :1;          ///< bit:  23     Band Select Clock Mode
        uint32_t _reserved_2      :8;          ///< bit:  24..31 RESERVED
    } b;
    uint32_t w;
} ADF4351_Reg3_t;


/** @brief  Union type for the structure of Register4 in ADF4351
 */
typedef union
{
    struct
    {
        uint32_t ControlBits    :3;          ///< bit:  0.. 2  CONTROL BITS
        uint32_t OutPower       :2;          ///< bit:  3.. 4  Output Power
        uint32_t OutEnable      :1;          ///< bit:  5      RF Output Enable
        uint32_t AuxPower       :2;          ///< bit:  6.. 7  AUX Output Power
        uint32_t AuxEnable      :1;          ///< bit:  8      AUX Output Enable
        uint32_t AuxSel         :1;          ///< bit:  9      AUX Output Select
        uint32_t Mtld           :1;          ///< bit: 10      Mute Till Lock Detect (MTLD)
        uint32_t VcoPowerDown   :1;          ///< bit: 11      VCO Power-Down
        uint32_t BandClkDiv     :8;          ///< bit: 12..19  Band Select Clock Divider Value
        uint32_t RfDivSel       :3;          ///< bit: 20..22  RF Divider Select
        uint32_t Feedback       :1;          ///< bit: 23      Feedback Select
        uint32_t _reserved_0    :8;          ///< bit: 24..31  RESERVED
    } b;
    uint32_t w;
} ADF4351_Reg4_t;


/** @brief  Union type for the structure of Register5 in ADF4351
 */
typedef union
{
    struct
    {
        uint32_t ControlBits      :3;       ///< bit:  0.. 2     CONTROL BITS
        uint32_t _reserved_0      :16;      ///< bit:  3..18 RESERVED
        uint32_t _reserved_1      :2;       ///< bit: 19..20 RESERVED
        uint32_t _reserved_2      :1;       ///< bit: 21     RESERVED
        uint32_t LdPinMode        :2;       ///< bit: 22..23 LD Pin Mode
        uint32_t _reserved_3      :8;       ///< bit: 24..31 RESERVED
    } b;
    uint32_t w;
} ADF4351_Reg5_t;


/** @brief  Phase adjust type
 *      The phase adjust bit (Bit DB28) enables adjustment
 *      of the output phase of a given output frequency.
 */
typedef enum
{
    ADF4351_PHASE_ADJ_OFF,
    ADF4351_PHASE_ADJ_ON
} ADF4351_PHASE_ADJ_t;


/** @brief Prescaler Value  type
 *      The dual-modulus prescaler (P/P + 1), along with the INT, FRAC, and
 *        MOD values, determines the overall division ratio from the VCO
 *        output to the PFD input.
 */
typedef enum
{
    ADF4351_PRESCALER_4_5,        ///< Prescaler = 4/5: NMIN = 23
    ADF4351_PRESCALER_8_9         ///< Prescaler = 8/9: NMIN = 75
} ADF4351_PRESCALER_t;


/** @brief Low Noise and Low Spur Modes  type
 *      The noise mode allows the user to optimize a design either
 *    for improved spurious performance or for improved phase noise performance.
 */
typedef enum
{
    ADF4351_LOW_NOISE_MODE,
    ADF4351_LOW_SPUR_MODE = 3
} ADF4351_SPURNOISE_t;


/** @brief MUXOUT  type
 *      The on-chip multiplexer
 */
typedef enum
{
    ADF4351_MUX_THREESTATE,
    ADF4351_MUX_DVDD,
    ADF4351_MUX_DGND,
    ADF4351_MUX_RCOUNTER,
    ADF4351_MUX_NDIVIDER,
    ADF4351_MUX_ANALOGLOCK,
    ADF4351_MUX_DIGITALLOCK
} ADF4351_MUX_t;



/** @brief Disable/Enable  type
 *      various bits are Disable(0)/Enable(1) type
 */
typedef enum
{
    ADF4351_DISABLE,
    ADF4351_ENABLE
} ADF4351_ED_t;



/** @brief Charge Pump Current Setting  type
 *      This value should be set to the charge pump current
 *    that the loop filter is designed with
 */
typedef enum
{
    ADF4351_CPCURRENT_0_31,
    ADF4351_CPCURRENT_0_63,
    ADF4351_CPCURRENT_0_94,
    ADF4351_CPCURRENT_1_25,
    ADF4351_CPCURRENT_1_56,
    ADF4351_CPCURRENT_1_88,
    ADF4351_CPCURRENT_2_19,
    ADF4351_CPCURRENT_2_50,
    ADF4351_CPCURRENT_2_81,
    ADF4351_CPCURRENT_3_13,
    ADF4351_CPCURRENT_3_44,
    ADF4351_CPCURRENT_3_75,
    ADF4351_CPCURRENT_4_06,
    ADF4351_CPCURRENT_4_38,
    ADF4351_CPCURRENT_4_69,
    ADF4351_CPCURRENT_5_00
} ADF4351_CPCURRENT_t;



/** @brief Lock Detect Function  type
 *      The LDF controls the number of PFD cycles monitored by the lock detect
 *    circuit to ascertain whether lock has been achieved.
 */
typedef enum
{
    ADF4351_LDF_FRAC,
    ADF4351_LDF_INT
} ADF4351_LDF_t;



/** @brief  Lock Detect Precision type
 *      The lock detect precision  sets the comparison window in the lock detect circuit.
 */
typedef enum
{
    ADF4351_LDP_10NS,
    ADF4351_LDP_6NS
} ADF4351_LDP_t;


/** @brief Phase Detector Polarity  type
 *      Phase Detector Polarity
 */
typedef enum
{
    ADF4351_POLARITY_NEGATIVE, ///< For active filter with an inverting charac-teristic
    ADF4351_POLARITY_POSITIVE  ///< For passive loop filter or a noninverting active loop filter
} ADF4351_POLARITY_t;


/** @brief  Band Select Clock Mode type
 *
 */
typedef enum
{
    ADF4351_BANDCLOCK_LOW,
    ADF4351_BANDCLOCK_HIGH
} ADF4351_BANDCLOCK_t;


/** @brief  Antibacklash Pulse Width type
 *
 */
typedef enum
{
    ADF4351_ABP_6NS,
    ADF4351_ABP_3NS
} ADF4351_ABP_t;


/** @brief Clock Divider Mode  type
 *
 */
typedef enum
{
    ADF4351_CLKDIVMODE_OFF,
    ADF4351_CLKDIVMODE_FAST_LOCK,
    ADF4351_CLKDIVMODE_RESYNC
} ADF4351_CLKDIVMODE_t;


/** @brief Feedback Select   type
 *
 */
typedef enum
{
    ADF4351_FEEDBACK_DIVIDED,     ///< the signal is taken from the output of the output dividers
    ADF4351_FEEDBACK_FUNDAMENTAL  ///<  he signal is taken directly from the VCO
} ADF4351_FEEDBACK_t;



/** @brief RF Divider Select type
 *
 */
typedef enum
{
    ADF4351_RFDIV_1,
    ADF4351_RFDIV_2,
    ADF4351_RFDIV_4,
    ADF4351_RFDIV_8,
    ADF4351_RFDIV_16,
    ADF4351_RFDIV_32,
    ADF4351_RFDIV_64
} ADF4351_RFDIV_t;


/** @brief Reference Divider
 *
 */
typedef enum
{
    ADF4351_REFDIV_1,
    ADF4351_REFDIV_2
} ADF4351_REFDIV_t;


/** @brief Reference Doubler
 *
 */
typedef enum
{
    ADF4351_REFMUL_1,
    ADF4351_REFMUL_2
} ADF4351_REFMUL_t;



/** @brief VCO Power-Down  type
 *
 */
typedef enum
{
    ADF4351_VCO_POWERUP,
    ADF4351_VCO_POWERDOWN
} ADF4351_VCO_POWER_t;


/** @brief Output Power  type
 *
 */
typedef enum
{
    ADF4351_POWER_MINUS4DB,
    ADF4351_POWER_MINUS1DB,
    ADF4351_POWER_PLUS2DB,
    ADF4351_POWER_PLUS5DB
} ADF4351_POWER_t;



/** @brief Lock Detect Pin Operation  type
 *
 */
typedef enum
{
    ADF4351_LD_PIN_LOW,
    ADF4351_LD_PIN_DIGITAL_LOCK,
    ADF4351_LD_PIN_LOW_,
    ADF4351_LD_PIN_HIGH
} ADF4351_LD_PIN_t;


/** @brief  ADF4351 Driver Error codes
 */
typedef enum
{
    ADF4351_Err_None,                          ///< No error
    ADF4351_Err_PFD,                           ///< PFD max error check
    ADF4351_Err_BandSelFreqTooHigh,            ///< Band select frequency too high
    ADF4351_Err_InvalidRCounterValue,          ///< Invalid R couinter value
    ADF4351_Err_NoGCD_PhaseAdj,                ///< No GCD when Phase adjust enabled
    ADF4351_Err_RFoutTooHigh,                  ///< Output frequency too high
    ADF4351_Err_RFoutTooLow,                   ///< Output frequency too low
    ADF4351_Err_REFinTooHigh,                  ///< Reference input too high
    ADF4351_Err_InvalidN,                      ///< N out of range
    ADF4351_Err_InvalidMOD,                    ///< MOD out of range
    ADF4351_Warn_NotTuned,                     ///< PLL output could not be tuned to exact frequency
    ADF4351_Err_InvalidMODLowSpur,             ///< Min. MOD in low spur mode is 50
} ADF4351_ERR_t;

static ADF4351_ERR_t UpdateFrequencyRegisters(double RFout, double REFin, double OutputChannelSpacing, int gcd, int AutoBandSelectClock, double *RFoutCalc) __attribute__((unused));
static uint32_t ADF4351_GetRegisterBuf(int addr) __attribute__((unused));
static void ADF4351_SetRegisterBuf(int addr, uint32_t val);
static void ADF4351_ClearRegisterBuf(void) __attribute__((unused));
static ADF4351_ERR_t ADF4351_SetRcounterVal(uint16_t val) __attribute__((unused));

/**
 * @brief ADF4351 registers - local storage
 *
 */
static ADF4351_Reg0_t ADF4351_Reg0;
static ADF4351_Reg1_t ADF4351_Reg1;
static ADF4351_Reg2_t ADF4351_Reg2;
static ADF4351_Reg3_t ADF4351_Reg3;
static ADF4351_Reg4_t ADF4351_Reg4;
static ADF4351_Reg5_t ADF4351_Reg5;


/**  @brief Private Function Prototypes
  *
  */
static ADF4351_RFDIV_t ADF4351_Select_Output_Divider(double RFoutFrequency)
{
    // Select divider
    if (RFoutFrequency >= 2200000000.0) return ADF4351_RFDIV_1;
    if (RFoutFrequency < 2200000000.0) return ADF4351_RFDIV_2;
    if (RFoutFrequency < 1100000000.0) return ADF4351_RFDIV_4;
    if (RFoutFrequency < 550000000.0) return ADF4351_RFDIV_8;
    if (RFoutFrequency < 275000000.0) return ADF4351_RFDIV_16;
    if (RFoutFrequency < 137500000.0) return ADF4351_RFDIV_32;
    return ADF4351_RFDIV_64;
}

// greatest common denominator - Euclid algo w/o recursion
static uint32_t gcd_iter(uint32_t u, uint32_t v)
{
    uint32_t t;
    while (v)
    {
        t = u;
        u = v;
        v = t % v;
    }
    return u;
}


/**
  *  @brief Main function to calculate register values based on required PLL output
  *
  * @param  RFout:                                 Required PLL output frequency in Hz
  *                 REFin:                                Reference clock in Hz
  *                 OutputChannelSpacing:    Output channel spacing in Hz
  *                 gcd:                                    calculate CGD (1) or not (0)
  *                 AutoBandSelectClock:    automatic calc of band selection clock
  * @paramOut  RFoutCalc:                 Calculated actual output frequency in Hz
  * @retval 0=OK, or Error code (ADF4351_ERR_t)
  */
static ADF4351_ERR_t UpdateFrequencyRegisters(double RFout, double REFin, double OutputChannelSpacing, int gcd, int AutoBandSelectClock, double *RFoutCalc )
{
    uint16_t        OutputDivider;
    uint32_t        temp;
    double          PFDFreq;                    // PFD Frequency in Hz
    uint16_t        Rcounter;                   // R counter value
    int32_t         RefDoubler;                 // ref. doubler
    int32_t         RefD2;                      // ref. div 2
    double          N;
    uint16_t        INT,MOD,FRAC;
    uint32_t        D;
    double          BandSelectClockDivider;
    double          BandSelectClockFrequency;

    // Initial error and range check
    // Error >>>> Disable GCD calculation when phase adjust active
    //if (gcd && ADF4351_Reg1.b.PhaseAdjust) return ADF4351_Err_NoGCD_PhaseAdj;
    if (RFout > ADF4351_RFOUT_MAX) return    ADF4351_Err_RFoutTooHigh;
    if (RFout < ADF4351_RFOUTMIN) return ADF4351_Err_RFoutTooLow;
    if (REFin > ADF4351_REFINMAX) return    ADF4351_Err_REFinTooHigh;

    // Calculate N, INT, FRAC, MOD

    RefD2 = ADF4351_Reg2.b.RDiv2 + 1;          // 1 or 2
    RefDoubler = ADF4351_Reg2.b.RMul2 + 1;     // 1 or 2
    Rcounter = ADF4351_Reg2.b.RCountVal;
    PFDFreq = (REFin * RefDoubler / RefD2) / Rcounter;

    OutputDivider = (1U << ADF4351_Select_Output_Divider(RFout));


    if (ADF4351_Reg4.b.Feedback == 1) // fundamental
        N = ((RFout * OutputDivider) / PFDFreq);
    else                                        // divided
        N = (RFout / PFDFreq);

    INT = (uint16_t)N;
    MOD = (uint16_t)(round(/*1000 * */(PFDFreq / OutputChannelSpacing)));
    FRAC = (uint16_t)(round(((double)N - INT) * MOD));

    if (gcd)
    {
        D = gcd_iter((uint32_t)MOD, (uint32_t)FRAC);

        MOD = MOD / D;
        FRAC = FRAC / D;
    }

    if (MOD == 1)
        MOD = 2;

    *RFoutCalc = (((double)((double)INT + ((double)FRAC / (double)MOD)) * (double)PFDFreq / OutputDivider) * ((ADF4351_Reg4.b.Feedback == 1) ? 1 : OutputDivider));
    N = INT + (FRAC / MOD);

    // N is out of range!
    if ((N < 23) | (N > 65635)) return ADF4351_Err_InvalidN;
    if (MOD > 0x0fff) return ADF4351_Err_InvalidMOD;

    /* Check for PFD Max error, return error code if not OK */
    if ((PFDFreq > ADF5451_PFD_MAX) && (ADF4351_Reg3.b.BandSelMode == 0)) return ADF4351_Err_PFD;
    if ((PFDFreq > ADF5451_PFD_MAX) && (ADF4351_Reg3.b.BandSelMode == 1) && (FRAC != 0)) return ADF4351_Err_PFD;
    if ((PFDFreq > 90) && (ADF4351_Reg3.b.BandSelMode == 1) && (FRAC != 0))  return ADF4351_Err_PFD;
    if ((ADF4351_Reg2.b.LowNoiseSpur == ADF4351_LOW_SPUR_MODE) && (MOD < 50)) return ADF4351_Err_InvalidMODLowSpur;

//        Calculate Band Select Clock
    if (AutoBandSelectClock)
    {
        if (ADF4351_Reg3.b.BandSelMode == 0)   /// LOW
        {
            temp = (uint32_t)round(8 * PFDFreq);
            if ((8 * PFDFreq - temp) > 0)
                temp++;
            temp = (temp > 255) ? 255 : temp;
            BandSelectClockDivider = (double)temp;
        }
        else                                            // High
        {
            temp = (uint32_t)round(PFDFreq * 2);
            if ((2 * PFDFreq - temp) > 0)
                temp++;
            temp = (temp > 255) ? 255 : temp;
            BandSelectClockDivider = (double)temp;
        }
    }
    BandSelectClockFrequency = (PFDFreq / (uint32_t)BandSelectClockDivider);

    /* Check parameters */
    if (BandSelectClockFrequency > 500e3)  return ADF4351_Err_BandSelFreqTooHigh;  // 500kHz in fast mode
    if ((BandSelectClockFrequency > 125e3) & (ADF4351_Reg3.b.BandSelMode == 0))  return ADF4351_Err_BandSelFreqTooHigh;   // 125kHz in slow mode

    // So far so good, let's fill the registers

    ADF4351_Reg0.b.FracVal = (FRAC & 0x0fff);
    ADF4351_Reg0.b.IntVal = (INT & 0xffff);
    ADF4351_Reg1.b.ModVal = (MOD & 0x0fff);
    ADF4351_Reg4.b.BandClkDiv = BandSelectClockDivider;

    if (*RFoutCalc == RFout)
        return ADF4351_Err_None;
    else
        return ADF4351_Warn_NotTuned;            // PLL could not be tuned to exatly required frequency --- check the RFoutCalc foree exact value
}


/**
  *  @brief Returns current value from local register buffer
  *
  * @param  addr:                                 Register address
  * @retval register value
  */
static uint32_t ADF4351_GetRegisterBuf(int addr)
{
    switch (addr)
    {
    case 0 :
        return ADF4351_Reg0.w;
    case 1 :
        return ADF4351_Reg1.w;
    case 2 :
        return ADF4351_Reg2.w;
    case 3 :
        return ADF4351_Reg3.w;
    case 4 :
        return ADF4351_Reg4.w;
    case 5 :
        return ADF4351_Reg5.w;
    }
    return 0x00000007;            // invalid address
}

/**
  *  @brief Set local register buffer value
  *
  *  @param  addr:                                 Register address
  */
static void ADF4351_SetRegisterBuf(int addr, uint32_t val)
{
    switch (addr)
    {
    case 0 :
        ADF4351_Reg0.w = val;
    case 1 :
        ADF4351_Reg1.w = val;
    case 2 :
        ADF4351_Reg2.w = val;
    case 3 :
        ADF4351_Reg3.w = val;
    case 4 :
        ADF4351_Reg4.w = val;
    case 5 :
        ADF4351_Reg5.w = val;
    }
}


/**
  *  @brief Clear local register buffer values to 0
  *
  *  @param  none
  */
static void ADF4351_ClearRegisterBuf(void)
{
    int i;

    for (i = 0; i < 6; i++)
        ADF4351_SetRegisterBuf(i, 0);

}



/**
  *  @brief Set R counter value
  *
  * @param  val:                                 Counter value (1...1023)
  * @retval register value
  */
static ADF4351_ERR_t ADF4351_SetRcounterVal(uint16_t val)
{
    if ((val > 0) && (val < 1024))
    {
        ADF4351_Reg2.b.RCountVal = val;
        return ADF4351_Err_None;
    }
    else
        return ADF4351_Err_InvalidRCounterValue;
}

//------------------------------------------------------------------
// Driver API
//------------------------------------------------------------------

void ADF4351_Init(void)
{
    ADF4351_Reg0.b.ControlBits = 0U;
    ADF4351_Reg1.b.ControlBits = 1U;
    ADF4351_Reg2.b.ControlBits = 2U;
    ADF4351_Reg3.b.ControlBits = 3U;
    ADF4351_Reg4.b.ControlBits = 4U;
    ADF4351_Reg5.b.ControlBits = 5U;
    ADF4351_Reg5.b._reserved_1 = 3U;
    //TODO : send to SPI
}


void ADF4351_Off(void)
{
    //TODO
}

void ADF4351_SetF0(uint32_t fhz)
{
    //TODO
}

void ADF4351_SetLO(uint32_t fhz)
{
    //TODO
}
