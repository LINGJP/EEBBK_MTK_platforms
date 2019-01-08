/************************************************************************
* Copyright (C) 2012-2017, Focaltech Systems (R)￡?All Rights Reserved.
*
* File Name: Test_FT5822.c
*
* Author: Software Development Team, AE
*
* Created: 2015-07-14
*
* Abstract: test item for FT5822\FT5626\FT5726\FT5826B
*
************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>

#include "../include/focaltech_test_detail_threshold.h"
#include "../include/focaltech_test_supported_ic.h"
#include "../include/focaltech_test_main.h"
#include "../focaltech_test_config.h"

#if (FTS_CHIP_TEST_TYPE ==FT5822_TEST)

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/

/////////////////////////////////////////////////Reg
#define DEVIDE_MODE_ADDR    0x00
#define REG_LINE_NUM    0x01
#define REG_TX_NUM  0x02
#define REG_RX_NUM  0x03
#define REG_PATTERN_5422        0x53
#define REG_MAPPING_SWITCH      0x54
#define REG_TX_NOMAPPING_NUM        0x55
#define REG_RX_NOMAPPING_NUM      0x56
#define REG_NORMALIZE_TYPE      0x16
#define REG_ScCbBuf0    0x4E
#define REG_ScWorkMode  0x44
#define REG_ScCbAddrR   0x45
#define REG_RawBuf0 0x36
#define REG_WATER_CHANNEL_SELECT 0x09

#define REG_FREQUENCY           0x0A
#define REG_FIR                 0XFB
/*add by lmh start*/
#define MAX_NOISE_FRAMES                			200
#define REG_RELEASECODEID_H     				0xAE
#define REG_RELEASECODEID_L     				0xAF
#define REG_FW_PROCESS          				0x1A
/*add by lmh end*/

/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/
enum WaterproofType
{
    WT_NeedProofOnTest,
    WT_NeedProofOffTest,
    WT_NeedTxOnVal,
    WT_NeedRxOnVal,
    WT_NeedTxOffVal,
    WT_NeedRxOffVal,
};

/*add by lmh start*/
struct structCompareChannel
{
    unsigned char bCh1;
    unsigned char bCh2;
    int fResistanceValue;
    int iAdcValue;
};

enum NOISE_TYPE
{
    NT_AvgData = 0,
    NT_MaxData = 1,
    NT_MaxDevication = 2,
    NT_DifferData = 3,
    
};
/*add by lmh end*/

/*****************************************************************************
* Static variables
*****************************************************************************/

static int m_RawData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static int m_iTempRawData[TX_NUM_MAX * RX_NUM_MAX] = {0};
static unsigned char m_ucTempData[TX_NUM_MAX * RX_NUM_MAX*2] = {0};
static bool m_bV3TP = false;
static int m_DifferData[TX_NUM_MAX][RX_NUM_MAX] = {{0}};
static int m_absDifferData[TX_NUM_MAX][RX_NUM_MAX] = {{0}};
//static int RxLinearity[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
//static int TxLinearity[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
/*add by lmh start*/
static int RxLinearity[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static int TxLinearity[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static int m_NoiseData[TX_NUM_MAX][RX_NUM_MAX] = {{0}};
static int m_TempNoiseData[MAX_NOISE_FRAMES][RX_NUM_MAX * TX_NUM_MAX] = {{0,0}};
static int m_AvgData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static int m_iTempData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};//Two-dimensional
static int minHole[TX_NUM_MAX][RX_NUM_MAX] = {{0}};
static int maxHole[TX_NUM_MAX][RX_NUM_MAX] = {{0}};
/*add by lmh start*/




/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
extern struct stCfg_FT5822_BasicThreshold g_stCfg_FT5822_BasicThreshold;

/*****************************************************************************
* Static function prototypes
*****************************************************************************/
//////////////////////////////////////////////Communication function
static int StartScan(void);
static unsigned char ReadRawData(unsigned char Freq, unsigned char LineNum, int ByteNum, int *pRevBuffer);
static unsigned char GetPanelRows(unsigned char *pPanelRows);
static unsigned char GetPanelCols(unsigned char *pPanelCols);
static unsigned char GetTxSC_CB(unsigned char index, unsigned char *pcbValue);
static unsigned char GetRawData(void);
static unsigned char GetChannelNum(void);
static void Save_Test_Data(int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount);
static void ShowRawData(void);
static boolean GetTestCondition(int iTestType, unsigned char ucChannelValue);

static unsigned char GetChannelNumNoMapping(void);
static unsigned char SwitchToNoMapping(void);
/*add by lmh start*/
static unsigned char TestItem_NewWeakShortTest( bool* bTestResult );
static unsigned char TestItem_OldWeakShortTest( bool* bTestResult );
unsigned char WeakShort_GetGndClbData(int *iValue);
static unsigned char WeakShort_GetAdcData( int AllAdcDataLen, int *pRevBuffer, unsigned char chCmd  );
/*add by lmh start*/


boolean FT5822_StartTest(void);
int FT5822_get_test_data(char *pTestData);//pTestData, External application for memory, buff size >= 1024*80

unsigned char FT5822_TestItem_EnterFactoryMode(void);
unsigned char FT5822_TestItem_RawDataTest(bool * bTestResult);
unsigned char FT5822_TestItem_SCapRawDataTest(bool* bTestResult);
unsigned char FT5822_TestItem_SCapCbTest(bool* bTestResult);
unsigned char FT5822_TestItem_PanelDifferTest(bool * bTestResult);
/*add by lmh start*/
unsigned char FT5822_TestItem_NoiseTest(bool * bTestResult);
unsigned char FT5822_TestItem_WeakShortTest(bool * bTestResult);
unsigned char FT5822_TestItem_UniformityTest(bool * bTestResult);
/*add by lmh end*/


boolean GetWaterproofMode(int iTestType, unsigned char ucChannelValue);

/************************************************************************
* Name: FT5822_StartTest
* Brief:  Test entry. Determine which test item to test
* Input: none
* Output: none
* Return: Test Result, PASS or FAIL
***********************************************************************/
boolean FT5822_StartTest()
{
    bool bTestResult = true;
    bool bTempResult = 1;
    unsigned char ReCode=0;
    unsigned char ucDevice = 0;
    int iItemCount=0;

    //--------------1. Init part
    if (InitTest() < 0)
    {
        FTS_TEST_ERROR("[focal] Failed to init test.");
        return false;
    }

    //--------------2. test item
    if (0 == g_TestItemNum)
        bTestResult = false;

    for (iItemCount = 0; iItemCount < g_TestItemNum; iItemCount++)
    {
        m_ucTestItemCode = g_stTestItem[ucDevice][iItemCount].ItemCode;

        ///////////////////////////////////////////////////////FT5822_ENTER_FACTORY_MODE
        if (Code_FT5822_ENTER_FACTORY_MODE == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT5822_TestItem_EnterFactoryMode();
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
                break;//if this item FAIL, no longer test.
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT5822_RAWDATA_TEST
        if (Code_FT5822_RAWDATA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT5822_TestItem_RawDataTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }


        ///////////////////////////////////////////////////////FT5822_SCAP_CB_TEST
        if (Code_FT5822_SCAP_CB_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT5822_TestItem_SCapCbTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT5822_SCAP_RAWDATA_TEST
        if (Code_FT5822_SCAP_RAWDATA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT5822_TestItem_SCapRawDataTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }
        ///////////////////////////////////////////////////////FT5822_PANELDIFFER_TEST
        if (Code_FT5822_PANELDIFFER_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT5822_TestItem_PanelDifferTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }
		/* add by lmh start */
		///////////////////////////////////////////////////////FT5822_UNIFORMITY_TEST
        if (Code_FT5822_UNIFORMITY_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT5822_TestItem_UniformityTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT5822_Noise_TEST
        if (Code_FT5822_NOISE_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT5822_TestItem_NoiseTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT5822_WEAK_SHORT_CIRCUIT_TEST
        if (Code_FT5822_WEAK_SHORT_CIRCUIT_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
        {
            ReCode = FT5822_TestItem_WeakShortTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }
		/* add by lmh end */

    }

    //--------------3. End Part
    FinishTest();

    //--------------4. return result
    return bTestResult;
}
/************************************************************************
* Name: FT5822_get_test_data
* Brief:  get data of test result
* Input: none
* Output: pTestData, the returned buff
* Return: the length of test data. if length > 0, got data;else ERR.
***********************************************************************/
int FT5822_get_test_data(char *pTestData)
{
    if (NULL == pTestData)
    {
        FTS_TEST_ERROR(" pTestData == NULL ");
        return -1;
    }
    memcpy(pTestData, g_pStoreAllData, (g_lenStoreMsgArea+g_lenStoreDataArea));
    return (g_lenStoreMsgArea+g_lenStoreDataArea);

//return sprintf(pTestData,"Hello man!");
}

/************************************************************************
* Name: FT5822_TestItem_EnterFactoryMode
* Brief:  Check whether TP can enter Factory Mode, and do some thing
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT5822_TestItem_EnterFactoryMode(void)
{
    unsigned char ReCode = ERROR_CODE_INVALID_PARAM;
    int iRedo = 5;//If failed, repeat 5 times.
    int i ;
    unsigned char chPattern=0;

    SysDelay(150);
    for (i = 1; i <= iRedo; i++)
    {
        ReCode = EnterFactory();
        if (ERROR_CODE_OK != ReCode)
        {
            FTS_TEST_ERROR("Failed to Enter factory mode...");
            if (i < iRedo)
            {
                SysDelay(50);
                continue;
            }
        }
        else
        {
            break;
        }

    }
    SysDelay(300);


    if (ReCode != ERROR_CODE_OK)
    {
        return ReCode;
    }

    //After the success of the factory model, read the number of channels
    ReCode = GetChannelNum();

    ////////////set FIR￡?0￡oclose￡?1￡oopen
    //theDevice.m_cHidDev[m_NumDevice]->WriteReg(0xFB, 0);

    // to determine whether the V3 screen body
    ReCode = ReadReg( REG_PATTERN_5422, &chPattern );
    if (chPattern == 1)
    {
        m_bV3TP = true;
    }
    else
    {
        m_bV3TP = false;
    }

    return ReCode;
}
/************************************************************************
* Name: FT5822_TestItem_RawDataTest
* Brief:  TestItem: RawDataTest. Check if MCAP RawData is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT5822_TestItem_RawDataTest(bool * bTestResult)
{
    unsigned char ReCode = 0;
    bool btmpresult = true;
    int RawDataMin;
    int RawDataMax;
    unsigned char ucFre;
    unsigned char strSwitch = 0;
    unsigned char OriginValue = 0xff;
    int index = 0;
    int iRow, iCol;
    int iValue = 0;


    FTS_TEST_INFO("\n\n==============================Test Item: -------- Raw Data  Test \n");
    ReCode = EnterFactory();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //Determine whether for v3 TP first, and then read the value of the 0x54
    //and check the mapping type is right or not,if not, write the register
    //rawdata test mapping before mapping 0x54=1;after mapping 0x54=0;
    if (m_bV3TP)
    {
        ReCode = ReadReg( REG_MAPPING_SWITCH, &strSwitch );
        if (strSwitch != 0)
        {
            ReCode = WriteReg( REG_MAPPING_SWITCH, 0 );
            if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;
        }
    }

    //Line by line one after the rawdata value, the default 0X16=0
    ReCode = ReadReg( REG_NORMALIZE_TYPE, &OriginValue );// read the original value
    if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;


    if (g_ScreenSetParam.isNormalize == Auto_Normalize)
    {
        if (OriginValue != 1) //if original value is not the value needed,write the register to change
        {
            ReCode = WriteReg( REG_NORMALIZE_TYPE, 0x01 );
            if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;
        }
        //Set Frequecy High

        FTS_TEST_DBG( "\n=========Set Frequecy High\n" );
        ReCode = WriteReg( 0x0A, 0x81 );
        if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;

        FTS_TEST_DBG( "\n=========FIR State: ON");
        ReCode = WriteReg(0xFB, 1);//FIR OFF  0:close, 1:open
        if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;
        //change register value before,need to lose 3 frame data
        for (index = 0; index < 3; ++index )
        {
            ReCode = GetRawData();
        }

        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("\nGet Rawdata failed, Error Code: 0x%x",  ReCode);
            goto TEST_ERR;
        }

        ShowRawData();

        ////////////////////////////////To Determine RawData if in Range or not
        for (iRow = 0; iRow<g_ScreenSetParam.iTxNum; iRow++)
        {
            for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
            {
                if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
                RawDataMin = g_stCfg_MCap_DetailThreshold.RawDataTest_High_Min[iRow][iCol];
                RawDataMax = g_stCfg_MCap_DetailThreshold.RawDataTest_High_Max[iRow][iCol];
                iValue = m_RawData[iRow][iCol];
                if (iValue < RawDataMin || iValue > RawDataMax)
                {
                    btmpresult = false;
                    FTS_TEST_ERROR("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                                   iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
                }
            }
        }

        //////////////////////////////Save Test Data
        Save_Test_Data(m_RawData, 0, g_ScreenSetParam.iTxNum, g_ScreenSetParam.iRxNum, 2);
    }
    else
    {
        if (OriginValue != 0) //if original value is not the value needed,write the register to change
        {
            ReCode = WriteReg( REG_NORMALIZE_TYPE, 0x00 );
            if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;
        }

        ReCode =  ReadReg( 0x0A, &ucFre );
        if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;


        //Set Frequecy Low
        if (g_stCfg_FT5822_BasicThreshold.RawDataTest_SetLowFreq)
        {
            FTS_TEST_DBG("\n=========Set Frequecy Low");
            ReCode = WriteReg( 0x0A, 0x80 );
            if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;

            //FIR OFF  0:close, 1:open

            FTS_TEST_DBG("\n=========FIR State: OFF\n" );
            ReCode = WriteReg(0xFB, 0);
            if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;
            SysDelay(100);
            //change register value before,need to lose 3 frame data
            for (index = 0; index < 3; ++index )
            {
                ReCode = GetRawData();
            }

            if ( ReCode != ERROR_CODE_OK )
            {
                FTS_TEST_ERROR("\nGet Rawdata failed, Error Code: 0x%x",  ReCode);
                goto TEST_ERR;
            }
            ShowRawData();

            ////////////////////////////////To Determine RawData if in Range or not
            for (iRow = 0; iRow<g_ScreenSetParam.iTxNum; iRow++)
            {

                for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
                {
                    if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
                    RawDataMin = g_stCfg_MCap_DetailThreshold.RawDataTest_Low_Min[iRow][iCol];
                    RawDataMax = g_stCfg_MCap_DetailThreshold.RawDataTest_Low_Max[iRow][iCol];
                    iValue = m_RawData[iRow][iCol];
                    if (iValue < RawDataMin || iValue > RawDataMax)
                    {
                        btmpresult = false;
                        FTS_TEST_ERROR("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                                       iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
                    }
                }
            }

            //////////////////////////////Save Test Data
            Save_Test_Data(m_RawData, 0, g_ScreenSetParam.iTxNum, g_ScreenSetParam.iRxNum, 1);
        }


        //Set Frequecy High
        if ( g_stCfg_FT5822_BasicThreshold.RawDataTest_SetHighFreq )
        {

            FTS_TEST_DBG( "\n=========Set Frequecy High");
            ReCode = WriteReg( 0x0A, 0x81 );
            if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;

            //FIR OFF  0:close, 1:open

            FTS_TEST_DBG("\n=========FIR State: OFF\n" );
            ReCode = WriteReg(0xFB, 0);
            if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;
            SysDelay(100);
            //change register value before,need to lose 3 frame data
            for (index = 0; index < 3; ++index )
            {
                ReCode = GetRawData();
            }

            if ( ReCode != ERROR_CODE_OK )
            {
                FTS_TEST_ERROR("\nGet Rawdata failed, Error Code: 0x%x",  ReCode);
                if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;
            }
            ShowRawData();

            ////////////////////////////////To Determine RawData if in Range or not
            for (iRow = 0; iRow<g_ScreenSetParam.iTxNum; iRow++)
            {

                for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
                {
                    if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
                    RawDataMin = g_stCfg_MCap_DetailThreshold.RawDataTest_High_Min[iRow][iCol];
                    RawDataMax = g_stCfg_MCap_DetailThreshold.RawDataTest_High_Max[iRow][iCol];
                    iValue = m_RawData[iRow][iCol];
                    if (iValue < RawDataMin || iValue > RawDataMax)
                    {
                        btmpresult = false;
                        FTS_TEST_ERROR("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                                       iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
                    }
                }
            }

            //////////////////////////////Save Test Data
            Save_Test_Data(m_RawData, 0, g_ScreenSetParam.iTxNum, g_ScreenSetParam.iRxNum, 2);
        }

    }



    ReCode = WriteReg( REG_NORMALIZE_TYPE, OriginValue );//set the origin value
    if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;

    //set V3 TP the origin mapping value
    if (m_bV3TP)
    {
        ReCode = WriteReg( REG_MAPPING_SWITCH, strSwitch );
        if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;
    }

    TestResultLen += sprintf(TestResult+TestResultLen,"RawData Test is %s.\n", (btmpresult ? "OK" : "NG"));
    //-------------------------Result
    if ( btmpresult )
    {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n//RawData Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//RawData Test is NG!");
    }
    return ReCode;

TEST_ERR:

    * bTestResult = false;
    FTS_TEST_INFO("\n\n//RawData Test is NG!");
    TestResultLen += sprintf(TestResult+TestResultLen,"RawData Test is NG.\n");
    return ReCode;

}
/************************************************************************
* Name: FT5822_TestItem_SCapRawDataTest
* Brief:  TestItem: SCapRawDataTest. Check if SCAP RawData is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT5822_TestItem_SCapRawDataTest(bool * bTestResult)
{
    int i =0;
    int RawDataMin = 0;
    int RawDataMax = 0;
    int Value = 0;
    boolean bFlag = true;
    unsigned char ReCode = 0;
    boolean btmpresult = true;
    int iMax=0;
    int iMin=0;
    int iAvg=0;
    int ByteNum=0;
    unsigned char wc_value = 0;//waterproof channel value
    unsigned char ucValue = 0;
    int iCount = 0;
//   int ibiggerValue = 0;

    FTS_TEST_INFO("\n\n==============================Test Item: -------- Scap RawData Test \n");
    //-------1.Preparatory work
    //in Factory Mode
    ReCode = EnterFactory();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //get waterproof channel setting, to check if Tx/Rx channel need to test
    ReCode = ReadReg( REG_WATER_CHANNEL_SELECT, &wc_value );
    if (ReCode != ERROR_CODE_OK) goto TEST_ERR;

    //If it is V3 pattern, Get Tx/Rx Num again
    ReCode= SwitchToNoMapping();
    if (ReCode != ERROR_CODE_OK) goto TEST_ERR;

    //-------2.Get SCap Raw Data, Step:1.Start Scanning; 2. Read Raw Data
    ReCode = StartScan();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("Failed to Scan SCap RawData! ");
        goto TEST_ERR;
    }
    for (i = 0; i < 3; i++)
    {
        memset(m_iTempRawData, 0, sizeof(m_iTempRawData));

        //water rawdata
        ByteNum = (g_ScreenSetParam.iTxNum + g_ScreenSetParam.iRxNum)*2;
        ReCode = ReadRawData(0, 0xAC, ByteNum, m_iTempRawData);
        if (ReCode != ERROR_CODE_OK)goto TEST_ERR;
        memcpy( m_RawData[0+g_ScreenSetParam.iTxNum], m_iTempRawData, sizeof(int)*g_ScreenSetParam.iRxNum );
        memcpy( m_RawData[1+g_ScreenSetParam.iTxNum], m_iTempRawData + g_ScreenSetParam.iRxNum, sizeof(int)*g_ScreenSetParam.iTxNum );

        //No water rawdata
        ByteNum = (g_ScreenSetParam.iTxNum + g_ScreenSetParam.iRxNum)*2;
        ReCode = ReadRawData(0, 0xAB, ByteNum, m_iTempRawData);
        if (ReCode != ERROR_CODE_OK)goto TEST_ERR;
        memcpy( m_RawData[2+g_ScreenSetParam.iTxNum], m_iTempRawData, sizeof(int)*g_ScreenSetParam.iRxNum );
        memcpy( m_RawData[3+g_ScreenSetParam.iTxNum], m_iTempRawData + g_ScreenSetParam.iRxNum, sizeof(int)*g_ScreenSetParam.iTxNum );
    }


    //-----3. Judge

    //Waterproof ON
    bFlag=GetTestCondition(WT_NeedProofOnTest, wc_value);
    if (g_stCfg_FT5822_BasicThreshold.SCapRawDataTest_SetWaterproof_ON && bFlag )
    {
        iCount = 0;
        RawDataMin = g_stCfg_FT5822_BasicThreshold.SCapRawDataTest_ON_Min;
        RawDataMax = g_stCfg_FT5822_BasicThreshold.SCapRawDataTest_ON_Max;
        iMax = -m_RawData[0+g_ScreenSetParam.iTxNum][0];
        iMin = 2 * m_RawData[0+g_ScreenSetParam.iTxNum][0];
        iAvg = 0;
        Value = 0;


        bFlag=GetTestCondition(WT_NeedRxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Rx in Waterproof-ON:");
        for ( i = 0; bFlag && i < g_ScreenSetParam.iRxNum; i++ )
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[0][i] == 0 )      continue;
            RawDataMin = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_ON_Min[0][i];
            RawDataMax = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_ON_Max[0][i];
            Value = m_RawData[0+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;
            if (iMax < Value) iMax = Value; //find the Max value
            if (iMin > Value) iMin = Value; //fine the min value
            if (Value > RawDataMax || Value < RawDataMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }


        bFlag=GetTestCondition(WT_NeedTxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Tx in Waterproof-ON:");
        for (i = 0; bFlag && i < g_ScreenSetParam.iTxNum; i++)
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[1][i] == 0 )      continue;
            RawDataMin = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_ON_Min[1][i];
            RawDataMax = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_ON_Max[1][i];
            Value = m_RawData[1+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;
            if (iMax < Value) iMax = Value; //find the Max value
            if (iMin > Value) iMin = Value; //fine the min value
            if (Value > RawDataMax || Value < RawDataMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }
        if (0 == iCount)
        {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        }
        else
            iAvg = iAvg/iCount;

        FTS_TEST_DBG("SCap RawData in Waterproof-ON, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        //ibiggerValue = g_ScreenSetParam.iTxNum>g_ScreenSetParam.iRxNum?g_ScreenSetParam.iTxNum:g_ScreenSetParam.iRxNum;
        Save_Test_Data(m_RawData, g_ScreenSetParam.iTxNum+0, 2, g_ScreenSetParam.iRxNum, 1);
    }

    //Waterproof OFF
    bFlag=GetTestCondition(WT_NeedProofOffTest, wc_value);
    if (g_stCfg_FT5822_BasicThreshold.SCapRawDataTest_SetWaterproof_OFF && bFlag)
    {
        iCount = 0;
        RawDataMin = g_stCfg_FT5822_BasicThreshold.SCapRawDataTest_OFF_Min;
        RawDataMax = g_stCfg_FT5822_BasicThreshold.SCapRawDataTest_OFF_Max;
        iMax = -m_RawData[2+g_ScreenSetParam.iTxNum][0];
        iMin = 2 * m_RawData[2+g_ScreenSetParam.iTxNum][0];
        iAvg = 0;
        Value = 0;

        bFlag=GetTestCondition(WT_NeedRxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Rx in Waterproof-OFF:");
        for (i = 0; bFlag && i < g_ScreenSetParam.iRxNum; i++)
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[0][i] == 0 )      continue;
            RawDataMin = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_OFF_Min[0][i];
            RawDataMax = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_OFF_Max[0][i];
            Value = m_RawData[2+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;

            //FTS_TEST_DBG("zaxzax3 Value %d RawDataMin %d  RawDataMax %d  ",  Value, RawDataMin, RawDataMax);
            //strTemp += str;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > RawDataMax || Value < RawDataMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }

        bFlag=GetTestCondition(WT_NeedTxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Tx in Waterproof-OFF:");
        for (i = 0; bFlag && i < g_ScreenSetParam.iTxNum; i++)
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[1][i] == 0 )      continue;

            Value = m_RawData[3+g_ScreenSetParam.iTxNum][i];
            RawDataMin = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_OFF_Min[1][i];
            RawDataMax = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_OFF_Max[1][i];
            //FTS_TEST_DBG("zaxzax4 Value %d RawDataMin %d  RawDataMax %d  ",  Value, RawDataMin, RawDataMax);
            iAvg += Value;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > RawDataMax || Value < RawDataMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }
        if (0 == iCount)
        {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        }
        else
            iAvg = iAvg/iCount;

        FTS_TEST_DBG("SCap RawData in Waterproof-OFF, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        //ibiggerValue = g_ScreenSetParam.iTxNum>g_ScreenSetParam.iRxNum?g_ScreenSetParam.iTxNum:g_ScreenSetParam.iRxNum;
        Save_Test_Data(m_RawData, g_ScreenSetParam.iTxNum+2, 2, g_ScreenSetParam.iRxNum, 2);
    }
    //-----4. post-stage work
    if (m_bV3TP)
    {
        ReCode = ReadReg( REG_MAPPING_SWITCH, &ucValue );
        if (0 !=ucValue )
        {
            ReCode = WriteReg( REG_MAPPING_SWITCH, 0 );
            SysDelay(10);
            if ( ReCode != ERROR_CODE_OK)
            {
                FTS_TEST_DBG("Failed to switch mapping type!\n ");
                btmpresult = false;
            }
        }

        //Only self content will be used before the Mapping, so the end of the test items, need to go after Mapping
        GetChannelNum();
    }

    TestResultLen += sprintf(TestResult+TestResultLen,"SCap RawData Test is %s.\n", (btmpresult ? "OK" : "NG"));
    //-----5. Test Result
    if ( btmpresult )
    {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n//SCap RawData Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//SCap RawData Test is NG!");
    }
    return ReCode;

TEST_ERR:
    * bTestResult = false;
    FTS_TEST_INFO("\n\n//SCap RawData Test is NG!");
    TestResultLen += sprintf(TestResult+TestResultLen,"SCap RawData Test is NG.\n");
    return ReCode;
}





/************************************************************************
* Name: FT5822_TestItem_SCapCbTest
* Brief:  TestItem: SCapCbTest. Check if SCAP Cb is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT5822_TestItem_SCapCbTest(bool* bTestResult)
{
    int i,/* j, iOutNum,*/index,Value,CBMin,CBMax;
    boolean bFlag = true;
    unsigned char ReCode;
    boolean btmpresult = true;
    int iMax, iMin, iAvg;
    unsigned char wc_value = 0;
    unsigned char ucValue = 0;
    int iCount = 0;
//   int ibiggerValue = 0;

    FTS_TEST_DBG("\n\n==============================Test Item: -----  Scap CB Test \n");
    //-------1.Preparatory work
    //in Factory Mode
    ReCode = EnterFactory();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //get waterproof channel setting, to check if Tx/Rx channel need to test
    ReCode = ReadReg( REG_WATER_CHANNEL_SELECT, &wc_value );
    if (ReCode != ERROR_CODE_OK) goto TEST_ERR;

    //If it is V3 pattern, Get Tx/Rx Num again
    bFlag= SwitchToNoMapping();
    if ( bFlag )
    {
        FTS_TEST_ERROR("Failed to SwitchToNoMapping! ");
        goto TEST_ERR;
    }

    //-------2.Get SCap Raw Data, Step:1.Start Scanning; 2. Read Raw Data
    ReCode = StartScan();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("Failed to Scan SCap RawData! ");
        goto TEST_ERR;
    }


    for (i = 0; i < 3; i++)
    {
        memset(m_RawData, 0, sizeof(m_RawData));
        memset(m_ucTempData, 0, sizeof(m_ucTempData));

        //waterproof CB
        ReCode = WriteReg( REG_ScWorkMode, 1 );//ScWorkMode:  1:waterproof 0:Non-waterproof
        ReCode = StartScan();
        ReCode = WriteReg( REG_ScCbAddrR, 0 );
        ReCode = GetTxSC_CB( g_ScreenSetParam.iTxNum + g_ScreenSetParam.iRxNum + 128, m_ucTempData );
        for ( index = 0; index < g_ScreenSetParam.iRxNum; ++index )
        {
            m_RawData[0 + g_ScreenSetParam.iTxNum][index]= m_ucTempData[index];
        }
        for ( index = 0; index < g_ScreenSetParam.iTxNum; ++index )
        {
            m_RawData[1 + g_ScreenSetParam.iTxNum][index] = m_ucTempData[index + g_ScreenSetParam.iRxNum];
        }

        //Non-waterproof rawdata
        ReCode = WriteReg( REG_ScWorkMode, 0 );//ScWorkMode:  1:waterproof 0:Non-waterproof
        ReCode = StartScan();
        ReCode = WriteReg( REG_ScCbAddrR, 0 );
        ReCode = GetTxSC_CB( g_ScreenSetParam.iRxNum + g_ScreenSetParam.iTxNum + 128, m_ucTempData );
        for ( index = 0; index < g_ScreenSetParam.iRxNum; ++index )
        {
            m_RawData[2 + g_ScreenSetParam.iTxNum][index]= m_ucTempData[index];
        }
        for ( index = 0; index < g_ScreenSetParam.iTxNum; ++index )
        {
            m_RawData[3 + g_ScreenSetParam.iTxNum][index] = m_ucTempData[index + g_ScreenSetParam.iRxNum];
        }

        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("Failed to Get SCap CB!");
        }
    }

    if (ReCode != ERROR_CODE_OK) goto TEST_ERR;

    //-----3. Judge

    //Waterproof ON
    bFlag=GetTestCondition(WT_NeedProofOnTest, wc_value);
    if (g_stCfg_FT5822_BasicThreshold.SCapCbTest_SetWaterproof_ON && bFlag)
    {
        FTS_TEST_DBG("SCapCbTest in WaterProof On Mode:  ");

        iMax = -m_RawData[0+g_ScreenSetParam.iTxNum][0];
        iMin = 2 * m_RawData[0+g_ScreenSetParam.iTxNum][0];
        iAvg = 0;
        Value = 0;
        iCount = 0;


        bFlag=GetTestCondition(WT_NeedRxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("SCap CB_Rx:  ");
        for ( i = 0; bFlag && i < g_ScreenSetParam.iRxNum; i++ )
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[0][i] == 0 )      continue;
            CBMin = g_stCfg_MCap_DetailThreshold.SCapCbTest_ON_Min[0][i];
            CBMax = g_stCfg_MCap_DetailThreshold.SCapCbTest_ON_Max[0][i];
            Value = m_RawData[0+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;

            if (iMax < Value) iMax = Value; //find the Max Value
            if (iMin > Value) iMin = Value; //find the Min Value
            if (Value > CBMax || Value < CBMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, CBMin, CBMax);
            }
            iCount++;
        }


        bFlag=GetTestCondition(WT_NeedTxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("SCap CB_Tx:  ");
        for (i = 0; bFlag &&  i < g_ScreenSetParam.iTxNum; i++)
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[1][i] == 0 )      continue;
            CBMin = g_stCfg_MCap_DetailThreshold.SCapCbTest_ON_Min[1][i];
            CBMax = g_stCfg_MCap_DetailThreshold.SCapCbTest_ON_Max[1][i];
            Value = m_RawData[1+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, CBMin, CBMax);
            }
            iCount++;
        }

        if (0 == iCount)
        {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        }
        else
            iAvg = iAvg/iCount;

        FTS_TEST_DBG("SCap CB in Waterproof-ON, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        //ibiggerValue = g_ScreenSetParam.iTxNum>g_ScreenSetParam.iRxNum?g_ScreenSetParam.iTxNum:g_ScreenSetParam.iRxNum;
        Save_Test_Data(m_RawData, g_ScreenSetParam.iTxNum+0, 2, g_ScreenSetParam.iRxNum, 1);
    }

    bFlag=GetTestCondition(WT_NeedProofOffTest, wc_value);
    if (g_stCfg_FT5822_BasicThreshold.SCapCbTest_SetWaterproof_OFF && bFlag)
    {
        FTS_TEST_DBG("SCapCbTest in WaterProof OFF Mode:  ");
        iMax = -m_RawData[2+g_ScreenSetParam.iTxNum][0];
        iMin = 2 * m_RawData[2+g_ScreenSetParam.iTxNum][0];
        iAvg = 0;
        Value = 0;
        iCount = 0;


        bFlag=GetTestCondition(WT_NeedRxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("SCap CB_Rx:  ");
        for (i = 0; bFlag &&  i < g_ScreenSetParam.iRxNum; i++)
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[0][i] == 0 )      continue;
            CBMin = g_stCfg_MCap_DetailThreshold.SCapCbTest_OFF_Min[0][i];
            CBMax = g_stCfg_MCap_DetailThreshold.SCapCbTest_OFF_Max[0][i];
            Value = m_RawData[2+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;

            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, CBMin, CBMax);
            }
            iCount++;
        }


        bFlag=GetTestCondition(WT_NeedTxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("SCap CB_Tx:  ");
        for (i = 0; bFlag && i < g_ScreenSetParam.iTxNum; i++)
        {
            //if( m_ScapInvalide[1][i] == 0 )      continue;
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[1][i] == 0 )      continue;
            CBMin = g_stCfg_MCap_DetailThreshold.SCapCbTest_OFF_Min[1][i];
            CBMax = g_stCfg_MCap_DetailThreshold.SCapCbTest_OFF_Max[1][i];
            Value = m_RawData[3+g_ScreenSetParam.iTxNum][i];

            iAvg += Value;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, CBMin, CBMax);
            }
            iCount++;
        }

        if (0 == iCount)
        {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        }
        else
            iAvg = iAvg/iCount;

        FTS_TEST_DBG("SCap CB in Waterproof-OFF, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        //ibiggerValue = g_ScreenSetParam.iTxNum>g_ScreenSetParam.iRxNum?g_ScreenSetParam.iTxNum:g_ScreenSetParam.iRxNum;
        Save_Test_Data(m_RawData, g_ScreenSetParam.iTxNum+2, 2, g_ScreenSetParam.iRxNum, 2);
    }
    //-----4. post-stage work
    if (m_bV3TP)
    {
        ReCode = ReadReg( REG_MAPPING_SWITCH, &ucValue );
        if (0 != ucValue )
        {
            ReCode = WriteReg( REG_MAPPING_SWITCH, 0 );
            SysDelay(10);
            if ( ReCode != ERROR_CODE_OK)
            {
                FTS_TEST_DBG("Failed to switch mapping type!\n ");
                btmpresult = false;
            }
        }

        //Only self content will be used before the Mapping, so the end of the test items, need to go after Mapping
        GetChannelNum();
    }

    TestResultLen += sprintf(TestResult+TestResultLen,"SCap CB Test is %s.\n", (btmpresult ? "OK" : "NG"));
    //-----5. Test Result

    if ( btmpresult )
    {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n//SCap CB Test Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//SCap CB Test Test is NG!");
    }
    return ReCode;

TEST_ERR:

    * bTestResult = false;
    FTS_TEST_INFO("\n\n//SCap CB Test Test is NG!");
    TestResultLen += sprintf(TestResult+TestResultLen,"SCap CB Test is NG.\n");
    return ReCode;
}
unsigned char FT5822_TestItem_PanelDifferTest(bool * bTestResult)
{
    int index = 0;
    int iRow = 0, iCol = 0;
    int iValue = 0;
    unsigned char ReCode = 0;
    bool btmpresult = true;
    int iMax, iMin; //, iAvg;
    int maxValue=0;
    int minValue=32767;
    int AvgValue = 0;
    int InvalidNum=0;
    int i = 0,  j = 0;


    unsigned char OriginRawDataType = 0xff;
    unsigned char OriginFrequecy = 0xff;
    unsigned char OriginFirState = 0xff;


    FTS_TEST_INFO("\r\n\r\n\r\n==============================Test Item: -------- Panel Differ Test  \r\n\r\n");

    ReCode = EnterFactory();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    FTS_TEST_DBG("\r\n=========Set Auto Equalization:\r\n");
    ReCode = ReadReg( REG_NORMALIZE_TYPE, &OriginRawDataType);//Read the original value
    if ( ReCode != ERROR_CODE_OK )
    {
        btmpresult = false;
        goto TEST_ERR;
    }

    ReCode = WriteReg( REG_NORMALIZE_TYPE, 0x00 );
    SysDelay(10);
    if ( ReCode != ERROR_CODE_OK )
    {
        btmpresult = false;
        FTS_TEST_ERROR( "\r\nWrite reg failed\r\n" );
        goto TEST_ERR;
    }

    //设置高频点
    FTS_TEST_DBG("\r\n=========Set Frequecy High\r\n");
    ReCode = ReadReg( REG_FREQUENCY, &OriginFrequecy);//Read the original value
    if ( ReCode != ERROR_CODE_OK )
    {
        btmpresult = false;
        goto TEST_ERR;
    }

    ReCode = WriteReg( 0x0A, 0x81);
    SysDelay(10);

    FTS_TEST_DBG("\r\n=========FIR State: OFF\r\n");
    ReCode = ReadReg( 0xFB, &OriginFirState);//Read the original value
    if ( ReCode != ERROR_CODE_OK )
    {
        btmpresult = false;
        goto TEST_ERR;
    }
    ReCode = WriteReg( 0xFB, 0);
    SysDelay(10);
    if ( ReCode != ERROR_CODE_OK )
    {
        FTS_TEST_ERROR("\r\nFailed to Write Fir Reg!\r\n ");
        btmpresult = false;
        goto TEST_ERR;
    }

    //Previously changed the register required to lose three frame data, the fourth frame is the use of the data
    for ( index = 0; index < 4; ++index )
    {
        ReCode = GetRawData();
        if ( ReCode != ERROR_CODE_OK )
        {
            btmpresult = false;
            goto TEST_ERR;
        }
    }

    ////Differ is RawData的1/10
    for ( i = 0; i < g_ScreenSetParam.iTxNum; i++)
    {
        for ( j= 0; j < g_ScreenSetParam.iRxNum; j++)
        {
            m_DifferData[i][j] = m_RawData[i][j]/10;
        }
    }

    ////////////////////////////////To show value
#if 1
    FTS_TEST_DBG("PannelDiffer :\n");
    for (iRow = 0; iRow<g_ScreenSetParam.iTxNum; iRow++)
    {
        FTS_TEST_DBG("\nRow%2d:    ", iRow+1);
        for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
        {
            //  if(g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue;//Invalid Node

            iValue = m_DifferData[iRow][iCol];
            FTS_TEST_DBG("%4d,  ", iValue);
        }
        FTS_TEST_DBG("\n" );
    }
    FTS_TEST_DBG("\n" );
#endif


    ///whether threshold is in range
    ////////////////////////////////To Determine  if in Range or not

    for (iRow = 0; iRow<g_ScreenSetParam.iTxNum; iRow++) //  iRow = 1 ???
    {
        for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
        {
            if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node

            iValue = m_DifferData[iRow][iCol];
            iMin =  g_stCfg_MCap_DetailThreshold.PanelDifferTest_Min[iRow][iCol];
            iMax = g_stCfg_MCap_DetailThreshold.PanelDifferTest_Max[iRow][iCol];

            if (iValue < iMin || iValue > iMax)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Out Of Range.  Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n", \
                               iRow+1, iCol+1, iValue, iMin, iMax);
            }
        }
    }

    ///////////////////////////  end determine

    ////////////////////
    ////>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>get test data ,and save to .csv file
    FTS_TEST_DBG("PannelDiffer ABS:\n");
    for ( i = 0; i <  g_ScreenSetParam.iTxNum; i++)
    {
        FTS_TEST_DBG("\n");
        for ( j = 0; j <  g_ScreenSetParam.iRxNum; j++)
        {

            FTS_TEST_DBG("%ld,", abs(m_DifferData[i][j]));
            m_absDifferData[i][j] = abs(m_DifferData[i][j]);

            if ( NODE_AST_TYPE == g_stCfg_MCap_DetailThreshold.InvalidNode[i][j] || NODE_INVALID_TYPE == g_stCfg_MCap_DetailThreshold.InvalidNode[i][j])
            {
                InvalidNum++;
                continue;
            }
            maxValue = max(maxValue,m_DifferData[i][j]);
            minValue = min(minValue,m_DifferData[i][j]);
            AvgValue += m_DifferData[i][j];
        }
    }
    FTS_TEST_DBG("\n");
    Save_Test_Data(m_absDifferData, 0, g_ScreenSetParam.iTxNum, g_ScreenSetParam.iRxNum, 1);
    ////<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<get test data ,and save to .csv file

    AvgValue = AvgValue/( g_ScreenSetParam.iTxNum*g_ScreenSetParam.iRxNum - InvalidNum);
    FTS_TEST_DBG("PanelDiffer:Max: %d, Min: %d, Avg: %d ", maxValue, minValue,AvgValue);

    ReCode = WriteReg( REG_NORMALIZE_TYPE, OriginRawDataType );//set to original value
    ReCode = WriteReg( 0x0A, OriginFrequecy );//set to original value
    ReCode = WriteReg( 0xFB, OriginFirState );//set to original value


    TestResultLen += sprintf(TestResult+TestResultLen,"Panel Differ Test is %s.\n", (btmpresult ? "OK" : "NG"));

    if ( btmpresult )
    {
        *bTestResult = true;
        FTS_TEST_INFO("		//Panel Differ Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("		//Panel Differ Test is NG!");
    }
    return ReCode;

TEST_ERR:

    * bTestResult = false;
    FTS_TEST_INFO("		//Panel Differ Test is NG!");
    TestResultLen += sprintf(TestResult+TestResultLen,"Panel Differ Test is NG.\n");
    return ReCode;
}

/*add by lmh start*/
unsigned char FT5822_TestItem_NoiseTest(bool * bTestResult)
{
    unsigned char ReCode = ERROR_CODE_OK;
    bool btmpresult = true;
    unsigned char chValue = 0;
    bool bFWProcess=false;
    unsigned char chFir = 0,chFre = 0;
    unsigned char chSetFir=0;
    int iNoiseFrames=0;
    int iNoiseTime=0;
    bool bSampModeFrame=true;
    int index = 0;
    int i=0, j=0;
    int iRow = 0, iCol = 0;
    int iValue = 0;
    int * pTemp=NULL;
    int * pTemp1 = NULL;
    unsigned char chNoiseValue=0;
    int iNoiseMax = 0;
    unsigned char ReleaseIDL = 0xff,ReleaseIDH=0xff;
    unsigned short  ReleaseID = 0;
    int iNoiseValue=0;
    int maxValue = 0,minValue = 0,AvgValue=0, InvalidNum=0;
    int iRawdataMin=65535;
    unsigned char chGloveModeValue = 0;


    //enter Factory mode
    ReCode = EnterFactory();
    if ( ReCode != ERROR_CODE_OK )
    {
        FTS_TEST_ERROR("\r\nFailed to Enter work Mode!\r\n ");
        btmpresult = false;
    }
	
    if (g_stCfg_FT5822_BasicThreshold.bNoiseTest_GloveMode)
    {
        FTS_TEST_INFO("\r\n\r\n==============================Test Item: -------- Noise Test In Glove Mode\r\n\r\n");

        //写寄存器，进入手套模式
        ReCode = (( ReCode == ERROR_CODE_OK ) &&  WriteReg( 0xF6, 0x1 ));
        SysDelay( 10 );
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("\r\nFailed to Read or Write Reg!\r\n ");
            btmpresult = false;
            goto TEST_END;
        }
    }
    else
    {
        FTS_TEST_INFO("\r\n\r\n==============================Test Item: -------- Noise Test \r\n\r\n");
    }

    //如果读取1a寄存器的值为1，且选择FWmode，则使用FW中处理的noise数据
    ReCode = ReadReg( REG_FW_PROCESS, &chValue );
    if ((ReCode != ERROR_CODE_OK))
    {
        FTS_TEST_ERROR("\r\nFailed to read reg!\r\n ");
        btmpresult = false;
        goto TEST_END;
    }
    if ((true == g_stCfg_FT5822_BasicThreshold.bNoiseTest_FWMode) && (1 == chValue))
    {
        bFWProcess = true;
    }

    //从FW中读取diff数据;0x06:1读differ数据；0X06:0读rawdata数据
    ReCode =  WriteReg( 0x06, 0x01 );
    SysDelay( 200 );
    ReCode =  ReadReg( 0x06, &chValue );
    FTS_TEST_DBG("chValue = %d. \n", chValue);
    if ( ReCode != ERROR_CODE_OK || chValue != 1)
    {
        FTS_TEST_ERROR("\r\nFailed to Read or Write Reg!\r\n ");
        btmpresult = false;
    }
    //Fir：1
    ReCode = ReadReg(0xFB, &chFir);
    SysDelay( 10 );
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_DBG("\r\nchFir = %d;ReCode = %d; before rawdata test\r\n",chFir,ReCode);
    }
    ReCode = WriteReg(0xFB, 1);
    SysDelay( 100 );
    if ( ReCode != ERROR_CODE_OK )
    {
        FTS_TEST_ERROR("\r\nFailed to Read or Write Reg!\r\n ");
        btmpresult = false;
        goto TEST_END;
    }

    //频率为默认频率
    ReCode =ReadReg( 0x0A,&chFre );
    SysDelay( 10 );
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_DBG("\r\nFir = %d;ReCode = %d; before rawdata test\r\n", chFre, ReCode);
    }
    chSetFir = g_stCfg_FT5822_BasicThreshold.NoiseTest_SetFrequency;
    ReCode = WriteReg( 0x0A,chSetFir );
    SysDelay(20);
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\r\nFailed to write Reg.\r\n");
        goto TEST_END;
    }

    //如果切换了频点，开始采集的数据会有很大差异，故而需要舍弃一些数据
    if (chFre != chSetFir)
    {
        for ( i=0; i<10; i++)
        {
            StartScan();
            SysDelay( 20 );
        }
    }

    //////////////////////////////////////////////////////////求Noise -Start

    iNoiseFrames = g_stCfg_FT5822_BasicThreshold.NoiseTest_Frames;
    iNoiseTime = g_stCfg_FT5822_BasicThreshold.NoiseTest_Time;
    bSampModeFrame = g_stCfg_FT5822_BasicThreshold.NoiseTest_SampeMode == 0;

    memset(m_NoiseData, 0, sizeof(m_NoiseData));
    if (bFWProcess)
    {
    	 FTS_TEST_DBG("FW deal with noise");
		
        //对OX1C写入ini设置的帧数;对0X1B写1后读取FW处理后的noise数据
        ReCode = WriteReg( 0x1C, iNoiseFrames );
        SysDelay(10);
        if (ReCode != ERROR_CODE_OK)
        {
            FTS_TEST_ERROR("\r\nWrite reg failed.\r\n");
            btmpresult = false;
            goto TEST_END;
        }

        ReCode = WriteReg( 0x1B, 1 );
        SysDelay(20);
        if (ReCode != ERROR_CODE_OK)
        {
            FTS_TEST_ERROR("\r\nWrite reg failed.\r\n");
            btmpresult = false;
            goto TEST_END;
        }
        for (  index = 0; index < 3; index++ )
        {
            ReCode = GetRawData();

            if (ReCode == ERROR_CODE_OK) break;

            if (index >= 2)
            {
                if (ReCode != ERROR_CODE_OK)
                {
                    FTS_TEST_ERROR("\r\nGet Raw Data, Times: %d, Error Code: 0x%x", index+1, ReCode);
                    btmpresult = false;
                    goto TEST_END;
                }
            }
        }
        for ( i=0; i<TX_NUM_MAX; i++)
        {
            for ( j=0; j<RX_NUM_MAX; j++)
            {
                m_NoiseData[i][j] = m_RawData[i][j];
            }
        }
    }
    else
    {
        //丢弃几帧数据
        for ( index = 0; index < 3; index++ )
        {
            ReCode = GetRawData();
            if (index >= 2)
            {
                if (ReCode != ERROR_CODE_OK)
                {
                    FTS_TEST_ERROR("\r\nGet Raw Data, Times: %d, Error Code: 0x%x", index+1, ReCode);
                    btmpresult = false;
                    goto TEST_END;
                }
            }
        }
		
        memset(m_TempNoiseData, 0, sizeof(m_TempNoiseData));
        memset(m_NoiseData, 0, sizeof(m_NoiseData));
        for (i = 0; i < iNoiseFrames; i++)
        {
            ReCode = GetRawData();
            if ( ReCode != ERROR_CODE_OK ) goto TEST_END;

            for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
            {
                for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
                {
                    m_TempNoiseData[i][iRow*g_ScreenSetParam.iRxNum + iCol] = m_RawData[iRow][iCol];

                }
            }
        }

        //total
        for (i = 0; i < iNoiseFrames; i++)
        {
            for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
            {
                for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
                {
                    iValue = m_TempNoiseData[i][iRow*g_ScreenSetParam.iRxNum + iCol];
                    m_NoiseData[iRow][iCol] += iValue;
                }
            }
        }
        //avg
        for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
        {
            for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
            {
                m_AvgData[iRow][iCol]  = m_NoiseData[iRow][iCol]  / iNoiseFrames;
            }
        }


        //Caculate noise by Noise Mode
        if (NT_AvgData == g_stCfg_FT5822_BasicThreshold.NoiseTest_NoiseMode)
        {
            //Caculate the Avg Value of all nodes
            //sqrt
            memset(m_NoiseData, 0, sizeof(m_NoiseData));
            for (i = 0; i < iNoiseFrames; i++)
            {
                for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
                {
                    for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
                    {
                        iValue = focal_abs(m_TempNoiseData[i][iRow*g_ScreenSetParam.iRxNum + iCol]);
                        m_NoiseData[iRow][iCol] += (iValue)*(iValue);

                    }
                }
            }

            for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
            {
                for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
                {
                    m_NoiseData[iRow][iCol]  = SqrtNew(m_NoiseData[iRow][iCol]  / iNoiseFrames);
                    //FTS_TEST_DBG("m_NoiseData[%d][%d]=%d, ",(iRow+1),(iCol+1), m_NoiseData[iRow][iCol]  );


                }
            }

        }
        else if (NT_MaxData == g_stCfg_FT5822_BasicThreshold.NoiseTest_NoiseMode)
        {
            //Find the Max Value of all nodes
            memset(m_NoiseData, 0, sizeof(m_NoiseData));
            for (i = 0; i < iNoiseFrames; i++)
            {
                for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
                {
                    for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
                    {
                        iValue = focal_abs(m_TempNoiseData[i][iRow*g_ScreenSetParam.iRxNum + iCol]);
                        if (iValue > m_NoiseData[iRow][iCol])
                            m_NoiseData[iRow][iCol] = iValue;
                    }
                }
            }
        }
        else if (NT_MaxDevication == g_stCfg_FT5822_BasicThreshold.NoiseTest_NoiseMode)
        {
            //CaculateNoiseBaseOnMaxMin(iRawDataAvr, iNoiseFrames);
            memset(m_iTempData, 0xffff, sizeof(m_iTempData));//Save The Min Value
            memset(m_NoiseData, 0, sizeof(m_NoiseData));    //Save The Max Value
            for (i = 0; i < iNoiseFrames; i++)
            {
                for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
                {
                    for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
                    {
                        iValue = m_TempNoiseData[i][iRow*g_ScreenSetParam.iRxNum + iCol];
                        if (iValue < m_iTempData[iRow][iCol])
                            m_iTempData[iRow][iCol] = iValue;
                        if (iValue > m_NoiseData[iRow][iCol])
                            m_NoiseData[iRow][iCol] = iValue;
                    }
                }
            }
            //Caculate Devication value(Max -Min)
            for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
            {
                for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
                {
                    m_NoiseData[iRow][iCol]  -= m_iTempData[iRow][iCol];
                }
            }

        }
        else if (NT_DifferData == g_stCfg_FT5822_BasicThreshold.NoiseTest_NoiseMode)
        {
            //Caculate the Avg Value of all nodes
            memset(m_NoiseData, 0, sizeof(m_NoiseData));
            for (i = 0; i< iNoiseFrames; i++)
            {
                pTemp = m_TempNoiseData[i];
                pTemp1 = m_TempNoiseData[i + 1];
                for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
                {
                    for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
                    {

                        m_iTempData[iRow][iCol] = focal_abs( pTemp[iRow*g_ScreenSetParam.iRxNum+iCol] - pTemp1[iRow*g_ScreenSetParam.iRxNum+iCol]);

                    }
                }
            }
            for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
            {
                for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
                {
                    m_NoiseData[iRow][iCol]  = m_iTempData[iRow][iCol];
                }
            }

        }

    }

    if (g_stCfg_FT5822_BasicThreshold.bNoiseTest_GloveMode)
    {
        //read Reg:0XF7
        ReCode = ReadReg(0xF7, &chNoiseValue);
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("\r\nFailed to Read  Reg!\r\n ");
            btmpresult = false;
        }
        //目前没有手套模式了
        iNoiseMax = (g_stCfg_FT5822_BasicThreshold.GloveNoiseTest_Coefficient/100)*chNoiseValue;

    }
    else
    {
        if (!g_stCfg_FT5822_BasicThreshold.bNoiseThreshold_Choose)
        {
            if (bFWProcess)
            {
                ReCode = ReadReg(0x0D, &chNoiseValue);

                ReCode = EnterWork();
                SysDelay(200);
                if (ReCode != ERROR_CODE_OK)
                {
                    FTS_TEST_ERROR("\r\n\r\n// Failed to Enter work Mode. Error Code: %d", ReCode);
                    btmpresult = false;
                    goto TEST_END;
                }
                ReCode = ReadReg(REG_RELEASECODEID_L, &ReleaseIDL);
                SysDelay(5);
                ReCode = ReadReg(REG_RELEASECODEID_H, &ReleaseIDH);
                SysDelay(5);

                //如果ReleaseID >= 0x0104，则阈值需要乘以4
                ReleaseID = (ReleaseIDH<<8) + ReleaseIDL;
                if (ReleaseID >= 0x0104)
                {
                    iNoiseValue = chNoiseValue*4;

                }
                else
                {
                    iNoiseValue = chNoiseValue;

                }

                ReCode = EnterFactory();
                SysDelay(100);
                if ( ReCode != ERROR_CODE_OK )
                {
                    FTS_TEST_ERROR("\r\nFailed to Enter Factory!\r\n ");
                    btmpresult = false;
                    goto TEST_END;
                }

            }
            else
            {
                //read Reg:0X80, Work Mode
                SysDelay(10);
                ReCode = EnterWork();
                SysDelay(100);
                ReCode = ReadReg(0x80, &chNoiseValue);
                SysDelay(100);
                ReCode = EnterFactory();
                SysDelay(100);
                if ( ReCode != ERROR_CODE_OK )
                {
                    FTS_TEST_ERROR("\r\nFailed to Enter Factory!\r\n ");
                    btmpresult = false;
                    goto TEST_END;
                }
                //FW写入前，将0x80寄存器的值右移了2位，故而要再左移2位
                iNoiseValue = chNoiseValue*4;
            }

            FTS_TEST_DBG("iNoiseValue = %d. \n", iNoiseValue);
        }
        else
        {
            iNoiseMax = g_stCfg_FT5822_BasicThreshold.NoiseTest_Max_Value;
        }
    }
	
    if (!g_stCfg_FT5822_BasicThreshold.bNoiseThreshold_Choose)
    {
        for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
        {
            for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
            {
                maxHole[iRow][iCol] = g_stCfg_MCap_DetailThreshold.NoistTest_Coefficient[iRow][iCol]*iNoiseValue/100;
            }
        }
    }
    else
    {
        for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
        {
            for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
            {
                maxHole[iRow][iCol] = iNoiseMax;
            }
        }
    }

    memset(minHole, 0, sizeof(minHole));
    for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
    {
        for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
        {
            if ( (0 == g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol]) )
            {
                continue;
            }
            if (m_NoiseData[iRow][iCol] <  minHole[iRow][iCol]  || m_NoiseData[iRow][iCol] >  maxHole[iRow][iCol] )
            {
                btmpresult = false;
                FTS_TEST_ERROR("noise test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d)  ",  \
                             iRow+1, iCol+1, m_NoiseData[iRow][iCol], minHole[iRow][iCol], maxHole[iRow][iCol]);
            }
        }
    }

    ////>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>收集测试数据，存入CSV文件
    for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
    {
        for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
        {
            FTS_TEST_DBG("%02d,", m_NoiseData[iRow][iCol]);
            if ( (0 == g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol]) )
            {
                InvalidNum++;
                continue;
            }
            maxValue = max(maxValue,m_NoiseData[iRow][iCol]);
            minValue = min(minValue,m_NoiseData[iRow][iCol]);
            AvgValue += m_NoiseData[iRow][iCol];
        }
    }
    Save_Test_Data(m_NoiseData, 0,  g_ScreenSetParam.iTxNum, g_ScreenSetParam.iRxNum, 1 );

    if (g_ScreenSetParam.iTxNum* g_ScreenSetParam.iRxNum- InvalidNum > 0)
    {
        AvgValue = AvgValue/(g_ScreenSetParam.iTxNum*g_ScreenSetParam.iRxNum - InvalidNum);
    }
    FTS_TEST_DBG("\r\nNoise:Max: %02d, Min: %02d, Avg: %02d ", maxValue, minValue,AvgValue);
    ////<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<收集测试数据，存入CSV文件

    //在FIR=1，默认频率下,当Rawdata_min大于等于阈值时则PASS，小于该阈值时则NG。 范围3000~15000
    if (g_stCfg_FT5822_BasicThreshold.bNoiseTest_GloveMode)
    {
        //频率为默认频率
        ReCode = WriteReg( 0x0A,0x00 );

        //将differ数据转为Rawdata数据
        ReCode = ( ReCode == ERROR_CODE_OK ) &&(WriteReg( 0x06,/*chOldValue*/0x00 ));
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("\r\nFailed to write Reg!\r\n ");
        }

        //读取rawdata,丢一帧数据
        for (  index = 0; index < 3; index++ )
        {
            ReCode = GetRawData();
        }
        for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
        {
            for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
            {
                if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)
                {

                    continue;  //无效节点不测试
                }
                if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 2)
                {

                    continue; //按键区不判断
                }
                if (iRawdataMin>m_RawData[iRow][iCol])
                {
                    iRawdataMin = m_RawData[iRow][iCol];
                }

            }
        }
        FTS_TEST_DBG("\r\n\r\nGet Min_Rawdata is:%d; Set Min_Rawdata is %d.\r\n",iRawdataMin,g_stCfg_FT5822_BasicThreshold.NoiseTest_RawdataMin);

        if (iRawdataMin < g_stCfg_FT5822_BasicThreshold.NoiseTest_RawdataMin)
        {
            btmpresult=false;
            FTS_TEST_ERROR("\r\nGet Min Rawdata is less than Set min Rawdata!\r\n ");
        }

    }

    if (g_stCfg_FT5822_BasicThreshold.bNoiseTest_GloveMode)
    {
        //将手套模式恢复为正常模式
        ReCode =  WriteReg( 0xF6, chGloveModeValue );
        //将FIR恢复原值
        //ReCode = (ReCode == ERROR_CODE_OK) && theDevice.m_cHidDev[m_NumDevice]->WriteReg(0xFB, chFir);
        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("\r\nFailed to write Reg!\r\n ");
        }
    }

TEST_END:
    //频率为默认频率
    ReCode =WriteReg( 0x0A,0x00 );
    SysDelay(20);
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\r\n Failed to write Reg; ReCode = %d; \r\n",ReCode);
        btmpresult = false;
    }

    //将FIR恢复原值
    ReCode =  WriteReg(0xFB, chFir);
    SysDelay(50);
    if ( ReCode != ERROR_CODE_OK )
    {
        FTS_TEST_ERROR("\r\nFailed to write Reg!\r\n ");
        btmpresult = false;
    }

    if (bFWProcess)
    {
        //恢复正常的读differ数据
        ReCode =WriteReg( 0x1B, 0x00 );
        SysDelay(20);
        if (ReCode != ERROR_CODE_OK)
        {
            FTS_TEST_ERROR("\r\nWrite reg failed.\r\n");
            btmpresult = false;
        }
    }

    //从FW恢复读取Rawdata数据
    ReCode = WriteReg( 0x06,0x00 );
    SysDelay(200);
    if ( ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\r\nFailed to Read or Write Reg!\r\n ");
        btmpresult = false;
    }


    TestResultLen += sprintf(TestResult+TestResultLen,"Noise Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

    if (btmpresult)
    {
        FTS_TEST_INFO("\r\n\r\n//Noise Test is OK!\r\n");
        * bTestResult = true;
    }
    else
    {
        FTS_TEST_INFO("\r\n\r\n//Noise Test is NG!\r\n");
        * bTestResult = false;
    }

    return ReCode;
}



unsigned char FT5822_TestItem_WeakShortTest(bool * bTestResult)
{

    unsigned char ReCode = ERROR_CODE_OK;
    unsigned char  ReleaseIDH=0, ReleaseIDL=0, ReleaseID = 0;
    FTS_TEST_INFO("\r\n\r\n==============================Test Item: -------- Weak Short-Circuit Test \r\n\r\n");

    ReCode = EnterWork();
    SysDelay(200);
    if (ReCode != ERROR_CODE_OK)
    {

        FTS_TEST_ERROR("\r\n\r\n// Failed to Enter work Mode. Error Code: %d", ReCode);
        *bTestResult = false;
        goto TEST_END;
    }
    ReCode =ReadReg(REG_RELEASECODEID_L, &ReleaseIDL);
    SysDelay(2);
    ReCode = (ERROR_CODE_OK != ReCode) || ReadReg(REG_RELEASECODEID_H, &ReleaseIDH);
    SysDelay(2);
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_DBG("\r\nRead:AF:%d, AE:%d\r\n",ReleaseIDL,ReleaseIDH);
        *bTestResult = false;
        goto TEST_END;
    }
    //如果ReleaseID > 0x0102，则FW中默认是使用修改过弱短路的adc的方式
    ReleaseID = (ReleaseIDH<<8) + ReleaseIDL;
    if (ReleaseID > 0x0102)

    {
        ReCode = TestItem_NewWeakShortTest(bTestResult);
    }
    else
    {
        ReCode = TestItem_OldWeakShortTest(bTestResult);
    }


TEST_END:

    TestResultLen += sprintf(TestResult+TestResultLen,"Weak Short Test is %s. \n\n", (*bTestResult ? "OK" : "NG"));

    if ( *bTestResult )
    {
        FTS_TEST_INFO("\r\n\r\n//Weak Short Test is OK!");
    }
    else
    {
        FTS_TEST_INFO("\r\n\r\n//Weak Short Test is NG!");
    }

    return ReCode;
}


//High frequency, FIR:0 data
unsigned char FT5822_TestItem_UniformityTest(bool * bTestResult)
{
    unsigned char ReCode = ERROR_CODE_OK;
    bool btmpresult = true;
    unsigned char ucFre = 0;
    unsigned char FirValue = 0;
    //unsigned char FirValues = 0;
    //char str[512] = {0};

    int iMin = 100000;
    int iMax = -100000;
    int iDeviation = 0;
    int iRow = 0;
    int iCol = 0;
    int iUniform = 0;
    int index = 0;

    FTS_TEST_INFO("\n\n==============================Test Item: --------RawData Uniformity Test \n");
    //-------1.Preparatory work
    //in Factory Mode
    ReCode = EnterFactory();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_END;
    }

    ReCode =  ReadReg( REG_FREQUENCY, &ucFre );
    if (ReCode != ERROR_CODE_OK)
    {
        goto TEST_END;
    }

    //high frequency
    ReCode = WriteReg( REG_FREQUENCY, 0x81 );
    SysDelay(100);
    if (ReCode != ERROR_CODE_OK)
    {
        goto TEST_END;
    }

    ReCode =ReadReg(REG_FIR, &FirValue);
    if (ReCode != ERROR_CODE_OK)
    {
        goto TEST_END;
    }

    // fir = 0
    ReCode = WriteReg(REG_FIR, 0);
    SysDelay(100);
    if (ReCode != ERROR_CODE_OK)
    {
        goto TEST_END;
    }

    //Previously changed the register required to throw three frames of data
    for (index = 0; index < 3; ++index )
    {
        ReCode = GetRawData();
    }

    if ( g_stCfg_FT5822_BasicThreshold.Uniformity_CheckTx )
    {
        FTS_TEST_DBG("\n\n=========Check Tx Linearity \n");

        memset(TxLinearity,  0, sizeof(TxLinearity));

        for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; ++iRow )
        {
            for (iCol = 1; iCol < g_ScreenSetParam.iRxNum; ++iCol )
            {
                iDeviation = focal_abs( m_RawData[iRow][iCol] - m_RawData[iRow][iCol-1] );
                iMax = m_RawData[iRow][iCol]>m_RawData[iRow][iCol-1] ?m_RawData[iRow][iCol]:m_RawData[iRow][iCol-1] ;
                iMax = iMax ? iMax : 1;
                TxLinearity[iRow][iCol] = 100 * iDeviation / iMax;
            }
        }

        for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; ++iRow )
        {
            for (iCol = 1; iCol < g_ScreenSetParam.iRxNum; ++iCol )
            {
                if (0 == g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol]) continue;
                if (2 == g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol]) continue;

                if (TxLinearity[iRow][iCol]<MIN_HOLE_LEVEL ||
                    TxLinearity[iRow][iCol]>g_stCfg_FT5822_BasicThreshold.Uniformity_Tx_Hole)
                {
                    FTS_TEST_ERROR("Tx Linearity Out Of Range, TX=%d, RX=%d, TxLinearity=%d, Tx_Hole=%d.",   iCol, iRow, TxLinearity[iRow][iCol], g_stCfg_FT5822_BasicThreshold.Uniformity_Tx_Hole);

                    btmpresult = false;
                }
            }
        }

        Save_Test_Data(TxLinearity,  0, g_ScreenSetParam.iTxNum,  g_ScreenSetParam.iRxNum, 1);

    }

    if ( g_stCfg_FT5822_BasicThreshold.Uniformity_CheckRx )
    {
        FTS_TEST_DBG("\n\n=========Check Rx Linearity \n");

        for (iRow = 1; iRow < g_ScreenSetParam.iTxNum; ++iRow )
        {
            for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; ++iCol )
            {
                iDeviation = focal_abs( m_RawData[iRow][iCol] - m_RawData[iRow-1][iCol] );
                iMax = m_RawData[iRow][iCol]>m_RawData[iRow-1][iCol] ?m_RawData[iRow][iCol] : m_RawData[iRow-1][iCol] ;
                iMax = iMax ? iMax : 1;
                RxLinearity[iRow][iCol] = 100 * iDeviation / iMax;
            }
        }

        for (iRow = 1; iRow < g_ScreenSetParam.iTxNum; ++iRow )
        {
            for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; ++iCol )
            {
                if (0 == g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol]) continue;
                if (2 == g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol]) continue;

                if (RxLinearity[iRow][iCol]<MIN_HOLE_LEVEL ||
                    RxLinearity[iRow][iCol]>g_stCfg_FT5822_BasicThreshold.Uniformity_Rx_Hole)
                {
                    FTS_TEST_ERROR("Rx Linearity Out Of Range, TX=%d, RX=%d, RxLinearity=%d, Rx_Hole=%d.",   iCol, iRow, RxLinearity[iRow][iCol], g_stCfg_FT5822_BasicThreshold.Uniformity_Rx_Hole);

                    btmpresult = false;
                }
            }
        }

        Save_Test_Data(RxLinearity,  0, g_ScreenSetParam.iTxNum,  g_ScreenSetParam.iRxNum, 2);

    }

    if ( g_stCfg_FT5822_BasicThreshold.Uniformity_CheckMinMax )
    {
        FTS_TEST_DBG("\n\n=========Check Min/Max \n");
        iMin = 100000;
        iMax = -100000;

        for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; ++iRow )
        {
            for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; ++iCol )
            {
                if ( 0 == g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] )
                {
                    continue;
                }
                if ( 2 == g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] )
                {
                    continue;
                }
                if ( iMin> m_RawData[iRow][iCol] ) iMin= m_RawData[iRow][iCol] ;
                if ( iMax< m_RawData[iRow][iCol] ) iMax= m_RawData[iRow][iCol] ;
            }
        }
        iMax = !iMax ? 1 : iMax;
        iUniform = 100 * focal_abs(iMin) / focal_abs(iMax);

        FTS_TEST_DBG("\n\n Min: %d, Max: %d, , Get Value of Min/Max: %d.",   iMin, iMax, iUniform );

        if ( iUniform < g_stCfg_FT5822_BasicThreshold.Uniformity_MinMax_Hole )
        {
            btmpresult = false;
            FTS_TEST_ERROR("\n\n MinMax Out Of Range, Set Value: %d.",  g_stCfg_FT5822_BasicThreshold.Uniformity_MinMax_Hole );
        }
    }

    //restore the original frequency
    ReCode =  WriteReg( REG_FREQUENCY, ucFre );
    SysDelay(100);
    if (ReCode != ERROR_CODE_OK)
    {
        goto TEST_END;
    }
    // restore the original frequency, fir
    ReCode =  WriteReg( REG_FIR, FirValue );
    SysDelay(100);
    if (ReCode != ERROR_CODE_OK)
    {
        goto TEST_END;
    }


    TestResultLen += sprintf(TestResult+TestResultLen,"Uniformity Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

TEST_END:

    if ( btmpresult && ReCode == ERROR_CODE_OK )
    {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n\n\n//Uniformity Test is OK!\n");
    }
    else
    {
        *bTestResult = false;
        FTS_TEST_INFO("\n\n\n\n//Uniformity Test is NG!\n");
    }

    TestResultLen += sprintf(TestResult+TestResultLen,"Uniformity Test is NG. \n\n");

    return ReCode;
}

static unsigned char TestItem_NewWeakShortTest( bool* bTestResult )
{
    unsigned char ReCode = ERROR_CODE_COMM_ERROR;
    int iClbData_Ground = 0, iClbData_Mutual = 0, iOffset = 0;
    bool btmpresult = true;
    bool bIsWeakShortGND = false;
    bool bIsWeakShortMut = false;
    bool bPrintMsg = false;
    unsigned char iTxNum, iRxNum, iChannelNum;
    int iMaxTx = 35;
    int iAllAdcDataNum = 63;
    int *iAdcData  = NULL;
    int *iTmpAdcData  = NULL;
    int i = 0, j = 0;
    int iMin_CG = 0;
    int iMin_CC = 0;
    int iDoffset = 0, iDsen = 0, iDrefn = 0;
    int *fMShortResistance = NULL, *fGShortResistance = NULL, *fOriginResistance = NULL;
    int iCount = 0;
    unsigned char *bErrorCh = NULL;
    unsigned char *bMin70kCh = NULL ;
    int iErrorNum = 0;
    int iMin70kNum = 0;
    unsigned char *bWBuf = NULL;
    int fValue = 0;
    int iValue = 0;
    int iNowMin70kNum = 0;
    bool bIsUsed = false;
    int iNum = 0;
    unsigned char k;
    int staticValue=0;
    int iCmpChCount = 0;
    int iErrChCount = 0;
    int *iTempAdcData = NULL;
    int iAdcCount = 0;
    struct structCompareChannel *strtCmpCh = NULL;
    struct structCompareChannel *strtErrorCh = NULL;


    FTS_TEST_DBG("\r\n\r\n Enter New Weak Short-Circuit Test \r\n\r\n");


    ReCode = EnterFactory();
    if (ReCode != ERROR_CODE_OK)
    {

        FTS_TEST_ERROR("\r\n\r\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        btmpresult = false;
        goto TEST_END;
    }
    SysDelay(200);
    //TxNum寄存器：0x02(Read Only)
    //RxNum寄存器：0x03(Read Only)
    ReCode = ReadReg(0x02, &iTxNum);//Get Tx
    ReCode =    ReadReg(0x03, &iRxNum);//Get Rx
    if (ReCode != ERROR_CODE_OK)
    {
        btmpresult = false;
        goto TEST_END;
    }
    iChannelNum = iTxNum + iRxNum;
    iMaxTx = iTxNum;
    iAllAdcDataNum = 1 + (1 + iTxNum + iRxNum)*2;//总通道数 + 对地校准数据 + 通道间校准数据 + Offset

    bWBuf = fts_malloc((iChannelNum + 3)*sizeof(unsigned char ));
    iAdcData = fts_malloc((iAllAdcDataNum+1)*sizeof(int));
    iTmpAdcData = fts_malloc(iChannelNum*iChannelNum*sizeof(int));

    memset(iAdcData, 0, (iAllAdcDataNum+1));
    memset(iTmpAdcData, 0, (iChannelNum*iChannelNum));

    for ( i = 0; i < 1; i++)
    {
        ReCode = WeakShort_GetAdcData( iAllAdcDataNum*2, iAdcData, 0x01 );
        FTS_TEST_DBG("\r\n\r\n first. \n");

        SysDelay(50);
        if (ReCode != ERROR_CODE_OK)
        {
            btmpresult = false;
            FTS_TEST_ERROR("\r\n\r\n// Failed to Get Weak short Data. Error Code: %d",ReCode);
            goto TEST_END;
        }
    }

    iOffset = iAdcData[0];
    //iClbData_Ground = iAdcData[1];
    iClbData_Mutual = iAdcData[2 + iChannelNum];

#if 0   //print all Adc value
    ///////////////////////Channel and Ground
    ///////////////////////Channel and Channel
    for (i = 0; i < iAllAdcDataNum/*iChannelNum*/; i++)
    {
        if (i <= (iChannelNum + 1))
        {
            if (i == 0)
                FTS_TEST_DBG("\n\n\nOffset %02d: %4d,	\n", i, iAdcData[i]);
            else if (i == 1) /*if(i <= iMaxTx)*/
                FTS_TEST_DBG("Ground %02d: %4d,	\n", i, iAdcData[i]);
            else if (i <= (iMaxTx + 1) )
                FTS_TEST_DBG("Tx%02d: %4d,	", i-1, iAdcData[i]);
            else  if (i <= (iChannelNum + 1) )
                FTS_TEST_DBG("Rx%02d: %4d,	", i - iMaxTx-1, iAdcData[i]);

            if (i % 10 == 0)
                FTS_TEST_DBG("\n");

        }
        else
        {
            if (i == (iChannelNum + 2)   )
                FTS_TEST_DBG("\n\n\nMultual %02d: %4d,	\n", i, iAdcData[i]);
            else if (i <= (iMaxTx)+(iChannelNum + 2))
                FTS_TEST_DBG("Tx%02d: %4d,	", i - (iChannelNum + 2), iAdcData[i]);
            else  if (i < iAllAdcDataNum)
                FTS_TEST_DBG("Rx%02d: %4d,	", i - iMaxTx - (iChannelNum + 2), iAdcData[i]);

            if (i % 10 == 0)
                FTS_TEST_DBG("\n");

        }
    }
    FTS_TEST_DBG("\r\n");
#endif


    /*===================================================================
    通道间互短
    Rshort = (((2047+Doffset) - Drefn)/(Dsen - Drefn)*16k - 19k)*Kcal - 3.7k

    Drefn: 数组首个数值
    Doffset： Offset - 1024
    Dsen： 在阈值范围之外的所有ADC数值


    ===================================================================*/
    iMin_CG = g_stCfg_FT5822_BasicThreshold.WeakShortTest_CG;
    iMin_CC = g_stCfg_FT5822_BasicThreshold.WeakShortTest_CC;

    fMShortResistance = fts_malloc(iChannelNum*sizeof(int));
    memset(fMShortResistance,0, iChannelNum);
    fGShortResistance =  fts_malloc(iChannelNum*sizeof(int));
    memset(fGShortResistance,0, iChannelNum);
    fOriginResistance =  fts_malloc(iChannelNum*sizeof(int));
    memset(fOriginResistance,0, iChannelNum);

    iDoffset = iOffset - 1024;
    iDrefn = iClbData_Mutual;
    iCount = 0;
    for ( i = 0; i < iChannelNum; i++)
    {
        iDsen = iAdcData[i+iChannelNum+3];
        if (iDrefn - iDsen <= 0) //小于等于0，直接判PASS
        {
            fMShortResistance[i] = iMin_CC;
            continue;
        }

        //采用新的公式进行计算
        fMShortResistance[i] = (202 * (iDsen - iDoffset) + 79786) /(iDrefn - iDsen)/* - 3*/;
        if (fMShortResistance[i] < 0) fMShortResistance[i] = 0;

        if ((iMin_CC > fMShortResistance[i])) //小于等于0，直接判0欧短路
        {
            iCount++;

        }
    }

    //把最初的互短电阻值存储下来，作为进一步判断使用
    for ( i = 0; i < iChannelNum; i++)
    {
        fOriginResistance[i] = fMShortResistance[i];
    }

    if (iCount > 0)
    {
        bIsWeakShortMut = true;
    }

#if 1
    ///////////////////
    //挑出地短的异常通道   //只对出现互短的通道判断是否有地短
    if (bIsWeakShortMut)
    {
        /*复查地短，需要进行两轮检测，包括电阻值大于70K和电阻值小于70K两种情况。
        * 每一轮检测都需要校准值参与计算，但校准值只需要读取一次，
        * 故需要在下一步地短检测前先读取地短的校准值。
        */

        ReCode =  WeakShort_GetGndClbData(&iClbData_Ground);
        if (ERROR_CODE_OK != ReCode)
        {
            btmpresult = false;
            goto TEST_END;
        }

        bErrorCh = fts_malloc(iChannelNum*sizeof( unsigned char));
        bMin70kCh = fts_malloc(iChannelNum*sizeof( unsigned char));
        iErrorNum = 0;
        iMin70kNum = 0;

        for ( i = 0; i < iTxNum + iRxNum; i++)
        {
            if (fMShortResistance[i] < iMin_CC)
            {
                bErrorCh[iErrorNum] = (unsigned char)(i + 1);
                iErrorNum++;
            }
        }

        if (iErrorNum > 0)//出现异常通道
        {
            bWBuf[0] = 0xF8;
            bWBuf[1] = (unsigned char)iErrorNum;
            for ( i = 0; i < iErrorNum; i++)
            {
                bWBuf[2 + i] = bErrorCh[i];
            }

            //告知FW哪个通道出问题
            ReCode = Comm_Base_IIC_IO(  bWBuf, (unsigned short)(iErrorNum + 2), NULL, 0 );

            for ( i = 0; i < 1; i++) //读取ADC值
            {
                ReCode = WeakShort_GetAdcData( iErrorNum*2, iTmpAdcData, 0x02 );
                FTS_TEST_DBG("\r\n\r\n second. \n");
                SysDelay(50);
                if (ReCode != ERROR_CODE_OK)
                {
                    FTS_TEST_ERROR("\r\n\r\n// Failed to Get Weak short Data. Error Code: %d", ReCode);
                    btmpresult = false;
                    goto TEST_END;
                }
            }

#ifdef DEBUG_WEAKSHORT
            FTS_TEST_DBG("\r\n Ground ADCData:\r\n");
            for ( i=0; i<iErrorNum; i++)
            {
                FTS_TEST_DBG(" 0x%x,	", iTmpAdcData[i]);

            }
#endif

            iDrefn = iClbData_Ground;
            for ( i=0; i<iErrorNum; i++)
            {
                iDsen = iTmpAdcData[i];
                if (iDrefn - iDsen <= 0)//小于等于0，直接判PASS
                {
                    continue;
                }
                fValue = (202 * (iDsen - iDoffset) + 79786) / (iDrefn - iDsen) - 3;

                if (fValue < 0) fValue = 0;
                if (iMin_CG > fValue)
                {
                    fGShortResistance[bErrorCh[i] - 1] = fValue;//更新为复检之后的电阻值
                    iAdcData[bErrorCh[i] + 1] = iDsen;
                    bIsWeakShortGND = true;

                    if (fValue > 70)
                    {
                        if (bErrorCh[i] <= iTxNum)
                        {
                            FTS_TEST_DBG("Tx%d with GND",bErrorCh[i]);
                        }
                        else
                        {
                            FTS_TEST_DBG( "Rx%d with GND", (bErrorCh[i] - iTxNum) );
                        }

                        FTS_TEST_DBG(": %.02d(kΩ), ADC: %d", fValue, iDsen);

                        btmpresult = false;

                    }

                    //如果互短有小于70K的，存下来，再次判断
                    if (fValue < 70)
                    {
                        iNowMin70kNum = iMin70kNum;
                        bIsUsed = false;
                        for ( iNum = 0; iNum < iNowMin70kNum; iNum++)
                        {
                            if (bErrorCh[i] == bMin70kCh[iNum])
                            {
                                bIsUsed = true;
                                break;
                            }
                        }
                        if (!bIsUsed)
                        {
                            bMin70kCh[iMin70kNum] = bErrorCh[i];
                            iMin70kNum++;
                        }
                    }
                }

            }
        }

        if (iMin70kNum > 0)//出现电阻值小于70K的通道
        {
            bWBuf[0] = 0xF8;
            bWBuf[1] = (unsigned char)iMin70kNum;
            for ( i = 0; i < iMin70kNum; i++)
            {
                bWBuf[2 + i] = bMin70kCh[i];
            }

            //告知FW哪个通道出问题
            ReCode = Comm_Base_IIC_IO( bWBuf, (unsigned short)(iMin70kNum + 2), NULL, 0 );
            for ( i = 0; i < 1; i++)
            {
                ReCode = WeakShort_GetAdcData( iMin70kNum*2, iTmpAdcData, 0x03 );
                FTS_TEST_DBG("\r\n\r\n third. \n");
                SysDelay(50);
                if (ReCode != ERROR_CODE_OK)
                {
                    FTS_TEST_ERROR("\r\n\r\n// Failed to Get Weak short Data. Error Code: %d", ReCode);
                    btmpresult = false;
                    goto TEST_END;
                }
            }

            iDrefn = iClbData_Ground;
            for ( i = 0; i < iMin70kNum; i++)
            {
                iDsen = iTmpAdcData[i];
                if (iDrefn - iDsen <= 0)//小于等于0，直接判PASS
                {
                    continue;
                }

                fValue = (4 * (iDsen - iDoffset) + 1698) / (iDrefn - iDsen) - 2;

                if (fValue < 0) fValue = 0;
                if (iMin_CG > fValue)
                {
                    fGShortResistance[bMin70kCh[i] - 1] = fValue;//更新为复检之后的电阻值
                    iAdcData[bMin70kCh[i] + 1] = iDsen;
                    if (bMin70kCh[i] <= iTxNum)
                    {

                        FTS_TEST_DBG("Tx%d with GND",bMin70kCh[i]);
                    }
                    else
                    {

                        FTS_TEST_DBG( "Rx%d with GND", (bMin70kCh[i] - iTxNum) );
                    }

                    FTS_TEST_DBG(": %.02d(kΩ), ADC: %d", fValue, iDsen);
                    btmpresult = false;
                }
            }

        }
        if (bErrorCh != NULL)
        {
            fts_free(bErrorCh);
            bErrorCh = NULL;
        }
        if (bMin70kCh != NULL)
        {
            fts_free (bMin70kCh);
            bMin70kCh = NULL;
        }

    }

#endif

    //挑出互短的异常通道,如有小于70K的，再次复检
    if (bIsWeakShortMut /*&& bNeedToRecheck*/)
    {
        bErrorCh = fts_malloc(iChannelNum*sizeof(unsigned char));
        bMin70kCh =  fts_malloc(iChannelNum*sizeof(unsigned char));
        iErrorNum = 0;
        iMin70kNum = 0;

        for (k = 0; k < iTxNum + iRxNum; k++)
        {
            if (fOriginResistance[k] < iMin_CC)
            {
                bErrorCh[iErrorNum] = (unsigned char)(k + 1);
                iErrorNum++;
            }
        }
        if (iErrorNum > 1)//出现两个或两个以上异常通道
        {
            staticValue=0;
            iCmpChCount = 0;
            iErrChCount = 0;
            strtCmpCh = fts_malloc((iErrorNum * (iErrorNum - 1))*sizeof(struct structCompareChannel));
            strtErrorCh = fts_malloc((iErrorNum * (iErrorNum - 1) / 2)*sizeof(struct structCompareChannel));

            bWBuf[0] = 0xF8;
            bWBuf[1] = (unsigned char)iErrorNum+1;
            for ( i = 0; i < iErrorNum; i++)
            {
                bWBuf[2] = bErrorCh[i];
                for ( j=0; j< iErrorNum; j++)
                {
                    bWBuf[3 + j] = bErrorCh[j];
                }

                //告知FW哪个通道出问题
                ReCode =Comm_Base_IIC_IO(bWBuf, (unsigned short)(iErrorNum + 3), NULL, 0 );
                iTempAdcData = fts_malloc((iErrorNum-1+1)*sizeof(int));
                memset(iTempAdcData,0,(iErrorNum-1+1));
                for ( j = 0; j < 1; j++)
                {
                    ReCode = WeakShort_GetAdcData( /*iErrorNum **/ (iErrorNum - 1) * 2, iTempAdcData, 0x04 );
                    FTS_TEST_DBG("\r\n\r\n four. \n");
                    SysDelay(50);

                    if (ReCode != ERROR_CODE_OK)
                    {
                        FTS_TEST_ERROR("\r\n\r\n// Failed to Get Weak short Data. Error Code: %d", ReCode);
                        btmpresult = false;
                        goto TEST_END;
                    }
                }

                memcpy(iTmpAdcData+staticValue,iTempAdcData,sizeof(int)*(iErrorNum - 1));
                staticValue += (iErrorNum - 1);

	         if (iTempAdcData != NULL)
            	  {
                	 fts_free(iTempAdcData);
                	 iTempAdcData = NULL;
            	  }
		  
            }
#ifdef DEBUG_WEAKSHORT
            FTS_TEST_DBG("\r\nMutual AdcData:\r\n");
            for ( i=0; i<staticValue; i++)
            {
                FTS_TEST_DBG(" 0x%x, ",*(iTmpAdcData+i));
            }
#endif

            ////计算
            iDrefn = iClbData_Mutual;
            iAdcCount = 0;

	    //先把通道间的电阻值存到数据
            for ( i = 0; i < iErrorNum; i++)
            {
                for ( j = 0; j < iErrorNum; j++)
                {
                    if (bErrorCh[i] == bErrorCh[j]) continue;
                    iDsen = iTmpAdcData[iAdcCount];
                    iAdcCount++;
                    if (iDrefn - iDsen <= 0)//小于等于0，直接判PASS
                    {
                        continue;
                    }
                    fValue = (202 * (iDsen - iDoffset) + 79786) / (iDrefn - iDsen) - 3;
                    if (fValue < 0) fValue = 0;
                    strtCmpCh[iCmpChCount].bCh1 = bErrorCh[i];
                    strtCmpCh[iCmpChCount].bCh2 = bErrorCh[j];
                    strtCmpCh[iCmpChCount].fResistanceValue = fValue;
                    strtCmpCh[iCmpChCount].iAdcValue = iDsen;
                    iCmpChCount++;

                }
            }

            //从通道间的两个电阻值挑出最大的电阻值，作为此两通道间的电阻值，然后再判断是否小于预设阈值
            for ( i = 0; i < iCmpChCount; i++)
            {
                for (j = i + 1; j < iCmpChCount; j++)
                {
                    //找到此两个通道
                    if ((strtCmpCh[i].bCh1 == strtCmpCh[j].bCh2) && (strtCmpCh[i].bCh2 == strtCmpCh[j].bCh1))
                    {
                        //比较值大小
                        fValue = strtCmpCh[i].fResistanceValue;
                        iValue = strtCmpCh[i].iAdcValue;
                        if (strtCmpCh[i].fResistanceValue < strtCmpCh[j].fResistanceValue)
                        {
                            fValue = strtCmpCh[j].fResistanceValue;
                            iValue = strtCmpCh[j].iAdcValue;
                        }

                        //是否小于预设阈值
                        if (iMin_CC > fValue)
                        {
                            strtErrorCh[iErrChCount].bCh1 = strtCmpCh[i].bCh1;
                            strtErrorCh[iErrChCount].bCh2 = strtCmpCh[i].bCh2;
                            strtErrorCh[iErrChCount].fResistanceValue = fValue;
                            strtErrorCh[iErrChCount].iAdcValue = iValue;

                            fMShortResistance[strtErrorCh[iErrChCount].bCh1 - 1] = fValue;//更新为复检之后的电阻值
                            fMShortResistance[strtErrorCh[iErrChCount].bCh2 - 1] = fValue;//更新为复检之后的电阻值
                            iAdcData[strtErrorCh[iErrChCount].bCh1 + 2 + iChannelNum] = iValue;//更新为复检之后的ADC值
                            iAdcData[strtErrorCh[iErrChCount].bCh2 + 2 + iChannelNum] = iValue;//更新为复检之后的ADC值

                            if (strtErrorCh[iErrChCount].fResistanceValue > 70)
                            {
                                //打印互短通道信息
                                if (strtErrorCh[iErrChCount].bCh1 <= iTxNum)
                                {

                                    FTS_TEST_DBG("Tx%d", (strtErrorCh[iErrChCount].bCh1) );
                                }
                                else
                                {

                                    FTS_TEST_DBG( "Rx%d", (strtErrorCh[iErrChCount].bCh1 - iTxNum) );
                                }

                                if (strtErrorCh[iErrChCount].bCh2 <= iTxNum)
                                {
                                    FTS_TEST_DBG("Tx%d", (strtErrorCh[iErrChCount].bCh2) );
                                }
                                else
                                {

                                    FTS_TEST_DBG( "Rx%d", (strtErrorCh[iErrChCount].bCh2 - iTxNum) );
                                }

                                FTS_TEST_DBG(": %.02d(kΩ), ADC: %d", strtErrorCh[iErrChCount].fResistanceValue, strtErrorCh[iErrChCount].iAdcValue);
                                btmpresult = false;
                                bPrintMsg = true;
                            }

                            iErrChCount++;

                        }

                    }
                }

            }

            //如果互短有小于70K的，存下来，再次判断
            for ( i = 0; i < iErrChCount; i++)
            {
                if (strtErrorCh[i].fResistanceValue < 70)
                {
                    iNowMin70kNum = iMin70kNum;
                    bIsUsed = false;
                    for ( iNum = 0; iNum < iNowMin70kNum; iNum++)
                    {
                        if (strtErrorCh[i].bCh1 == bMin70kCh[iNum])
                        {
                            bIsUsed = true;
                            break;
                        }
                    }
                    if (!bIsUsed)
                    {
                        bMin70kCh[iMin70kNum] = strtErrorCh[i].bCh1;
                        iMin70kNum++;
                    }
                    bIsUsed = false;
                    for ( iNum = 0; iNum < iNowMin70kNum; iNum++)
                    {
                        if (strtErrorCh[i].bCh2 == bMin70kCh[iNum])
                        {
                            bIsUsed = true;
                            break;
                        }
                    }

                    if (!bIsUsed)
                    {
                        bMin70kCh[iMin70kNum] = strtErrorCh[i].bCh2;
                        iMin70kNum++;
                    }
                }
            }

            if (strtCmpCh != NULL)
            {
                fts_free(strtCmpCh);
                strtCmpCh = NULL;
            }
            if (strtErrorCh != NULL)
            {
                fts_free(strtErrorCh);
                strtErrorCh = NULL;
            }

        }

	//存在小于70K的，再次检测互短
        if (iMin70kNum > 0)
        {
            staticValue=0;
            iCmpChCount = 0;
            iErrChCount = 0;
            strtCmpCh = fts_malloc((iMin70kNum * (iMin70kNum - 1))*sizeof(struct structCompareChannel));
            strtErrorCh = fts_malloc((iMin70kNum * (iMin70kNum - 1) / 2)*sizeof(struct structCompareChannel));


            bWBuf[0] = 0xF8;
            bWBuf[1] = (unsigned char)iMin70kNum+1;
            for (i = 0; i < iMin70kNum; i++)
            {
                bWBuf[2] = bMin70kCh[i];

                for (j=0; j<iMin70kNum; j++)
                {
                    bWBuf[3+j] = bMin70kCh[j];
                }

                //告知FW哪个通道出问题
                ReCode = Comm_Base_IIC_IO( bWBuf, (unsigned short)(iMin70kNum + 3), NULL, 0 );

                iTempAdcData = fts_malloc((iMin70kNum-1)*sizeof(int));
                memset(iTempAdcData,0,(iMin70kNum-1));

                for ( i = 0; i < 1; i++)
                {
                    ReCode =WeakShort_GetAdcData( /*iMin70kNum **/ (iMin70kNum - 1) * 2, iTempAdcData, 0x05 );
                    FTS_TEST_DBG("\r\n\r\n five. \n");
                    SysDelay(50);

                    if (ReCode != ERROR_CODE_OK)
                    {
                        FTS_TEST_ERROR("\r\n\r\n// Failed to Get Weak short Data. Error Code: %d", ReCode);
                        btmpresult = false;
                        goto TEST_END;
                    }
                }
                memcpy(iTmpAdcData+staticValue,iTempAdcData,sizeof(int)*(iMin70kNum - 1));
                staticValue += (iMin70kNum - 1);

		  if (iTempAdcData != NULL)
            	 {
                	fts_free(iTempAdcData);
                	iTempAdcData = NULL;
            	 }
            }

#ifdef DEBUG_WEAKSHORT
            FTS_TEST_DBG("\r\nMutual AdcData_Min70K:\r\n");
            for ( i=0; i<staticValue; i++)
            {
                FTS_TEST_DBG(" 0x%x, ",*(iTmpAdcData+i));
            }
#endif

            iDrefn = iClbData_Mutual;
            iAdcCount = 0;

            //先把通道间的电阻值存到数据
            for ( i = 0; i < iMin70kNum; i++)
            {
                for ( j = 0; j < iMin70kNum; j++)
                {
                    if (bErrorCh[i] == bErrorCh[j]) continue;
                    iDsen = iTmpAdcData[iAdcCount];
                    iAdcCount++;
                    if (iDrefn - iDsen <= 0)//小于等于0，直接判PASS
                    {
                        continue;
                    }
                    fValue =(4* (iDsen - iDoffset) + 1698) / (iDrefn - iDsen) - 3;
                    if (fValue < 0) fValue = 0;
                    strtCmpCh[iCmpChCount].bCh1 = bErrorCh[i];
                    strtCmpCh[iCmpChCount].bCh2 = bErrorCh[j];
                    strtCmpCh[iCmpChCount].fResistanceValue = fValue;
                    strtCmpCh[iCmpChCount].iAdcValue = iDsen;
                    iCmpChCount++;
                }
            }

            //从通道间的两个电阻值挑出最大的电阻值，作为此两通道间的电阻值，然后再判断是否小于预设阈值
            for ( i = 0; i < iCmpChCount; i++)
            {
                for ( j = i; j < iCmpChCount; j++)
                {
                    //找到此两个通道，
                    if ((strtCmpCh[i].bCh1 == strtCmpCh[j].bCh2) && (strtCmpCh[i].bCh2 == strtCmpCh[j].bCh1))
                    {
                        //比较值大小
                        fValue = strtCmpCh[i].fResistanceValue;
                        iValue = strtCmpCh[i].iAdcValue;
                        if (strtCmpCh[i].fResistanceValue < strtCmpCh[j].fResistanceValue)
                        {
                            fValue = strtCmpCh[j].fResistanceValue;
                            iValue = strtCmpCh[j].iAdcValue;
                        }

                        //是否小于预设阈值
                        if (iMin_CC > fValue)
                        {
                            strtErrorCh[iErrChCount].bCh1 = strtCmpCh[i].bCh1;
                            strtErrorCh[iErrChCount].bCh2 = strtCmpCh[i].bCh2;
                            strtErrorCh[iErrChCount].fResistanceValue = fValue;
                            strtErrorCh[iErrChCount].iAdcValue = iValue;

                            fMShortResistance[strtErrorCh[iErrChCount].bCh1 - 1] = fValue;//更新为复检之后的电阻值
                            fMShortResistance[strtErrorCh[iErrChCount].bCh2 - 1] = fValue;//更新为复检之后的电阻值
                            iAdcData[strtErrorCh[iErrChCount].bCh1 + 2 + iChannelNum] = iValue;//更新为复检之后的ADC值
                            iAdcData[strtErrorCh[iErrChCount].bCh2 + 2 + iChannelNum] = iValue;//更新为复检之后的ADC值

                            //打印互短通道信息
                            if (strtErrorCh[iErrChCount].bCh1 <= iTxNum)
                            {
                                FTS_TEST_DBG("Tx%d", (strtErrorCh[iErrChCount].bCh1) );
                            }
                            else
                            {

                                FTS_TEST_DBG( "Rx%d", (strtErrorCh[iErrChCount].bCh1 - iTxNum) );
                            }

                            if (strtErrorCh[iErrChCount].bCh2 <= iTxNum)
                            {

                                FTS_TEST_DBG("Tx%d", (strtErrorCh[iErrChCount].bCh2) );
                            }
                            else
                            {

                                FTS_TEST_DBG( "Rx%d", (strtErrorCh[iErrChCount].bCh2 - iTxNum) );
                            }

                            FTS_TEST_DBG(": %.02d(kΩ), ADC: %d", strtErrorCh[iErrChCount].fResistanceValue, strtErrorCh[iErrChCount].iAdcValue);
                            iErrChCount++;
                            btmpresult = false;
                            bPrintMsg = true;
                        }
                    }
                }
            }

            if (strtCmpCh != NULL)
            {
                fts_free(strtCmpCh);
                strtCmpCh = NULL;
            }
            if (strtErrorCh != NULL)
            {
                fts_free(strtErrorCh);
                strtErrorCh = NULL;
            }
        }

        if (bErrorCh != NULL)
        {
            fts_free (bErrorCh);
            bErrorCh = NULL;
        }
        if (bMin70kCh != NULL)
        {
            fts_free (bMin70kCh);
            bMin70kCh = NULL;
        }
    }

#if 1
    //释放数组空间
    if (bWBuf != NULL)
    {
        fts_free(bWBuf);
        bWBuf = NULL;
    }
    if (iAdcData != NULL)
    {
        fts_free(iAdcData);
        iAdcData = NULL;
    }
    if (iTmpAdcData != NULL)
    {
        fts_free(iTmpAdcData);
        iTmpAdcData = NULL;
    }
    if (fMShortResistance != NULL)
    {
        fts_free(fMShortResistance);
        fMShortResistance = NULL;
    }
    if (fGShortResistance != NULL)
    {
        fts_free(fGShortResistance);
        fGShortResistance = NULL;
    }
    if (fOriginResistance != NULL)
    {
        fts_free(fOriginResistance);
        fOriginResistance = NULL;
    }
#endif

TEST_END:

    FTS_TEST_DBG(" ClbData_GND:%d, ClbData_Mutual:%d, Offset:%d",  iClbData_Ground, iClbData_Mutual, iOffset);
    if (bIsWeakShortGND && bIsWeakShortMut)
    {
        FTS_TEST_DBG("GND and Mutual Weak Short!");
    }
    else if (bIsWeakShortGND)
    {
        FTS_TEST_DBG("GND Weak Short!");
    }
    else if (bIsWeakShortMut)
    {
        FTS_TEST_DBG("Mutual Weak Short!");
    }
    else
    {
        FTS_TEST_DBG("No Short!");
    }

    if ( btmpresult )
    {

        * bTestResult = true;
    }
    else
    {

        * bTestResult = false;
    }

    return ReCode;

}
static unsigned char TestItem_OldWeakShortTest( bool* bTestResult )
{
    unsigned char ReCode = ERROR_CODE_COMM_ERROR;
    int iClbData_Ground = 0, iClbData_Mutual = 0, iOffset = 0;
    bool btmpresult = true;
    bool bIsWeakShortGND = false;
    bool bIsWeakShortMut = false;
    bool bPrintMsg = false;
    unsigned char iTxNum, iRxNum, iChannelNum;
    int iMaxTx = 35;
    int iAllAdcDataNum = 63;
    int *iAdcData  = NULL;
    int *iTmpAdcData  = NULL;
    int i = 0, j = 0;
    int iMin_CG = 0;
    int iMin_CC = 0;
    int iDoffset = 0, iDsen = 0, iDrefn = 0;
    int *fMShortResistance = NULL, *fGShortResistance = NULL, *fOriginResistance = NULL;
    int iCount = 0;
    unsigned char *bErrorCh = NULL;
    unsigned char *bMin70kCh = NULL ;
    int iErrorNum = 0;
    int iMin70kNum = 0;
    unsigned char *bWBuf = NULL;
    int fValue = 0;
    int iValue = 0;
    int iNowMin70kNum = 0;
    bool bIsUsed = false;
    int iNum = 0;
    unsigned char k;
    int iCmpChCount = 0;
    int iErrChCount = 0;
    int iAdcCount = 0;
    int tmp = 0;
    struct structCompareChannel *strtCmpCh = NULL;
    struct structCompareChannel *strtErrorCh = NULL;


    FTS_TEST_DBG("\r\n\r\n Enter Old Weak Short-Circuit Test \r\n\r\n");


    ReCode = EnterFactory();
    if (ReCode != ERROR_CODE_OK)
    {

        FTS_TEST_ERROR("\r\n\r\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        btmpresult = false;
        goto TEST_END;
    }
    SysDelay(200);
    //TxNum寄存器：0x02(Read Only)
    //RxNum寄存器：0x03(Read Only)
    ReCode = ReadReg(0x02, &iTxNum);//Get Tx
    ReCode =    ReadReg(0x03, &iRxNum);//Get Rx
    if (ReCode != ERROR_CODE_OK)
    {
        btmpresult = false;
        goto TEST_END;
    }
    iChannelNum = iTxNum + iRxNum;
    iMaxTx = iTxNum;
    iAllAdcDataNum = 1 + (1 + iTxNum + iRxNum)*2;//总通道数 + 对地校准数据 + 通道间校准数据 + Offset

    FTS_TEST_DBG("iAllAdcDataNum = %d", iAllAdcDataNum);

    bWBuf = fts_malloc((iChannelNum + 3)*sizeof(unsigned char ));
    iAdcData = fts_malloc((iAllAdcDataNum)*sizeof(int));
    iTmpAdcData = fts_malloc(iAllAdcDataNum*iAllAdcDataNum*sizeof(int));

    memset(iAdcData, 0, iAllAdcDataNum );
    memset(iTmpAdcData, 0, iAllAdcDataNum*iAllAdcDataNum);

    for ( i = 0; i < 1; i++)
    {
        ReCode = WeakShort_GetAdcData( iAllAdcDataNum*2, iAdcData, 0x01 );

        SysDelay(50);
        if (ReCode != ERROR_CODE_OK)
        {
            btmpresult = false;
            FTS_TEST_ERROR("\r\n\r\n// Failed to Get Weak short Data. Error Code: %d",ReCode);
            goto TEST_END;
        }
    }

#if 0
    ///////////////////////Channel and Ground
    ///////////////////////Channel and Channel
    for (i = 0; i < iAllAdcDataNum/*iChannelNum*/; i++)
    {
        if (i <= (iChannelNum + 1))
        {
            if (i == 0)
                FTS_TEST_DBG("\n\n\nOffset %02d: %4d,	\n", i, iAdcData[i]);
            else if (i == 1) /*if(i <= iMaxTx)*/
                FTS_TEST_DBG("Ground %02d: %4d,	\n", i, iAdcData[i]);
            else if (i <= (iMaxTx + 1) )
                FTS_TEST_DBG("Tx%02d: %4d,	", i-1, iAdcData[i]);
            else  if (i <= (iChannelNum + 1) )
                FTS_TEST_DBG("Rx%02d: %4d,	", i - iMaxTx-1, iAdcData[i]);
				
            if (i % 10 == 0)
                FTS_TEST_DBG("\n");

        }
        else
        {
            if (i == (iChannelNum + 2)   )
                FTS_TEST_DBG("\n\n\nMultual %02d: %4d,	\n", i, iAdcData[i]);
            else if (i <= (iMaxTx)+(iChannelNum + 2))
                FTS_TEST_DBG("Tx%02d: %4d,	", i - (iChannelNum + 2), iAdcData[i]);
            else  if (i < iAllAdcDataNum)
                FTS_TEST_DBG("Rx%02d: %4d,	", i - iMaxTx - (iChannelNum + 2), iAdcData[i]);
            if (i % 10 == 0)
                FTS_TEST_DBG("\n");

        }
    }
    FTS_TEST_DBG("\r\n");
#endif

    iOffset = iAdcData[0];
    //iClbData_Ground = iAdcData[1];
    iClbData_Mutual = iAdcData[2 + iChannelNum];

    /*===================================================================
    通道间互短
    Rshort = (((2047+Doffset) - Drefn)/(Dsen - Drefn)*16k - 19k)*Kcal - 3.7k

    Drefn: 数组首个数值
    Doffset： Offset - 1024
    Dsen： 在阈值范围之外的所有ADC数值


    ===================================================================*/
    iMin_CG = g_stCfg_FT5822_BasicThreshold.WeakShortTest_CG;
    iMin_CC = g_stCfg_FT5822_BasicThreshold.WeakShortTest_CC;

    fMShortResistance = fts_malloc(iChannelNum*sizeof(int));
    memset(fMShortResistance,0,iChannelNum );
    fGShortResistance =  fts_malloc(iChannelNum*sizeof(int));
    memset(fGShortResistance,0, iChannelNum);
    fOriginResistance =  fts_malloc(iChannelNum*sizeof(int));
    memset(fOriginResistance,0, iChannelNum);

    iDoffset = iOffset - 1024;
    iDrefn = iClbData_Mutual;
    iCount = 0;

    for ( i = 0; i < iChannelNum; i++)
    {
        iDsen = iAdcData[i+iChannelNum+3];
        if (iDrefn - iDsen <= 0) //小于等于0，直接判PASS
        {
            fMShortResistance[i] = iMin_CC;
            continue;
        }

        //采用新的公式进行计算
        fMShortResistance[i] = (202 * (iDsen - iDoffset) + 79786) / (iDrefn - iDsen)/* - 3*/;
        if (fMShortResistance[i] < 0) fMShortResistance[i] = 0;

        if ((iMin_CC > fMShortResistance[i])) //小于等于0，直接判0欧短路
        {
            tmp = fMShortResistance[i];
            iCount++;

        }
    }

    //把最初的互短电阻值存储下来，作为进一步判断使用
    for ( i = 0; i < iChannelNum; i++)
    {
        fOriginResistance[i] = fMShortResistance[i];
    }

    if (iCount > 0)
    {
        bIsWeakShortMut = true;
    }

    ///////////////////
    //挑出地短的异常通道   //只对出现互短的通道判断是否有地短
    if (bIsWeakShortMut)
    {
        /*复查地短，需要进行两轮检测，包括电阻值大于70K和电阻值小于70K两种情况。
        * 每一轮检测都需要校准值参与计算，但校准值只需要读取一次，
        * 故需要在下一步地短检测前先读取地短的校准值。
        */

        ReCode =  WeakShort_GetGndClbData(&iClbData_Ground);
        if (ERROR_CODE_OK != ReCode)
        {
            btmpresult = false;
            goto TEST_END;
        }

        bErrorCh = fts_malloc(iChannelNum*sizeof( unsigned char));
        bMin70kCh = fts_malloc(iChannelNum*sizeof( unsigned char));
        iErrorNum = 0;
        iMin70kNum = 0;

        for ( i = 0; i < iTxNum + iRxNum; i++)
        {
            if (fMShortResistance[i] < iMin_CC)
            {
                bErrorCh[iErrorNum] = (unsigned char)(i + 1);
                iErrorNum++;
            }
        }

        if (iErrorNum > 0)//出现异常通道
        {
            bWBuf[0] = 0xF8;
            bWBuf[1] = (unsigned char)iErrorNum;
            for ( i = 0; i < iErrorNum; i++)
            {
                bWBuf[2 + i] = bErrorCh[i];
            }

            //告知FW哪个通道出问题
            ReCode = Comm_Base_IIC_IO(  bWBuf, (unsigned short)(iErrorNum + 2), NULL, 0 );

            for ( i = 0; i < 1; i++) //读取ADC值
            {
                FTS_TEST_DBG("0x02 iErrorNum =  %d", iErrorNum);				
                ReCode = WeakShort_GetAdcData( iErrorNum*2, iTmpAdcData, 0x02 );
                SysDelay(50);
                if (ReCode != ERROR_CODE_OK)
                {
                    FTS_TEST_ERROR("\r\n\r\n// Failed to Get Weak short Data. Error Code: %d", ReCode);
                    btmpresult = false;
                    goto TEST_END;
                }
            }

#ifdef DEBUG_WEAKSHORT
            FTS_TEST_DBG("\r\n Ground ADCData:\r\n");
            for ( i=0; i<iErrorNum; i++)
            {
                FTS_TEST_DBG(" 0x%x,	", iTmpAdcData[i]);

            }
#endif
            iDrefn = iClbData_Ground;
            for ( i=0; i<iErrorNum; i++)
            {
                iDsen = iTmpAdcData[i];
                if (iDrefn - iDsen <= 0)//小于等于0，直接判PASS
                {
                    continue;
                }
                fValue = (202 * (iDsen - iDoffset) + 79786) / (iDrefn - iDsen) - 3;

                if (fValue < 0) fValue = 0;
                if (iMin_CG > fValue)
                {
                    fGShortResistance[bErrorCh[i] - 1] = fValue;//更新为复检之后的电阻值
                    iAdcData[bErrorCh[i] + 1] = iDsen;
                    bIsWeakShortGND = true;

		      FTS_TEST_DBG("GND fValue = %d", fValue );
                    if (fValue > 70)
                    {
                        if (bErrorCh[i] <= iTxNum)
                        {
                            FTS_TEST_DBG("Tx%d with GND",bErrorCh[i]);
                        }
                        else
                        {
                            FTS_TEST_DBG( "Rx%d with GND", (bErrorCh[i] - iTxNum) );
                        }

                        FTS_TEST_DBG(": %.02d(kΩ), ADC: %d", fValue, iDsen);

                        btmpresult = false;

                    }

                    //如果互短有小于70K的，存下来，再次判断
                    if (fValue < 70)
                    {
                        iNowMin70kNum = iMin70kNum;
                        bIsUsed = false;
                        for ( iNum = 0; iNum < iNowMin70kNum; iNum++)
                        {
                            if (bErrorCh[i] == bMin70kCh[iNum])
                            {
                                bIsUsed = true;
                                break;
                            }
                        }
                        if (!bIsUsed)
                        {
                            bMin70kCh[iMin70kNum] = bErrorCh[i];
                            iMin70kNum++;
                        }
                    }
                }

            }
        }

        //存在小于70K的，再次检测地短
        if (iMin70kNum > 0)//出现电阻值小于70K的通道
        {
            bWBuf[0] = 0xF8;
            bWBuf[1] = (unsigned char)iMin70kNum;
            for ( i = 0; i < iMin70kNum; i++)
            {
                bWBuf[2 + i] = bMin70kCh[i];
            }

            //告知FW哪个通道出问题
            ReCode = Comm_Base_IIC_IO( bWBuf, (unsigned short)(iMin70kNum + 2), NULL, 0 );
            for ( i = 0; i < 1; i++)
            {
                FTS_TEST_DBG("0x03 iMin70kNum =  %d", iMin70kNum);
                ReCode = WeakShort_GetAdcData( iMin70kNum*2, iTmpAdcData, 0x03 );
                SysDelay(50);
                if (ReCode != ERROR_CODE_OK)
                {
                    FTS_TEST_ERROR("\r\n\r\n// Failed to Get Weak short Data. Error Code: %d", ReCode);
                    btmpresult = false;
                    goto TEST_END;
                }
            }
	
#ifdef DEBUG_WEAKSHORT
		FTS_TEST_DBG("\r\nShort-GND ADCData:\r\n");
		for ( i=0; i<iMin70kNum; i++)
		{
			FTS_TEST_DBG(" 0x%x,	", iTmpAdcData[i]);
		}
#endif

            iDrefn = iClbData_Ground;
            for ( i = 0; i < iMin70kNum; i++)
            {
                iDsen = iTmpAdcData[i];
                if (iDrefn - iDsen <= 0)//小于等于0，直接判PASS
                {
                    continue;
                }

                fValue = (4 * (iDsen - iDoffset) + 1698) / (iDrefn - iDsen) - 2;

                if (fValue < 0) fValue = 0;
		  FTS_TEST_DBG("GND fValue = %d", fValue);		
		  FTS_TEST_DBG("iMin_CG = %d", iMin_CG);
                if (iMin_CG > fValue)
                {
                    fGShortResistance[bMin70kCh[i] - 1] = fValue;//更新为复检之后的电阻值
                    iAdcData[bMin70kCh[i] + 1] = iDsen;
                    if (bMin70kCh[i] <= iTxNum)
                    {

                        FTS_TEST_DBG("Tx%d with GND",bMin70kCh[i]);
                    }
                    else
                    {

                        FTS_TEST_DBG( "Rx%d with GND", (bMin70kCh[i] - iTxNum) );
                    }

                    FTS_TEST_DBG(": %.02d(kΩ), ADC: %d", fValue, iDsen);
                    btmpresult = false;
                }
            }
        }
        if (bErrorCh != NULL)
        {
            fts_free(bErrorCh);
            bErrorCh = NULL;
        }
        if (bMin70kCh != NULL)
        {
            fts_free (bMin70kCh);
            bMin70kCh = NULL;
        }

    }


    //挑出互短的异常通道,如有小于70K的，再次复检
    if (bIsWeakShortMut /*&& bNeedToRecheck*/)
    {
        bErrorCh = fts_malloc(iChannelNum*sizeof(unsigned char));
        bMin70kCh =  fts_malloc(iChannelNum*sizeof(unsigned char));
        iErrorNum = 0;
        iMin70kNum = 0;

        for (k = 0; k < iTxNum + iRxNum; k++)
        {
            if (fOriginResistance[k] < iMin_CC)
            {
                bErrorCh[iErrorNum] = (unsigned char)(k + 1);
                iErrorNum++;
            }
        }

        if (iErrorNum > 1)//出现两个或两个以上异常通道
        {

            iCmpChCount = 0;
            iErrChCount = 0;
            strtCmpCh = fts_malloc((iErrorNum * (iErrorNum - 1))*sizeof(struct structCompareChannel));
            strtErrorCh = fts_malloc((iErrorNum * (iErrorNum - 1) / 2)*sizeof(struct structCompareChannel));

            bWBuf[0] = 0xF8;
            bWBuf[1] = (unsigned char)iErrorNum;
            for ( i = 0; i < iErrorNum; i++)
            {
                bWBuf[2 + i] = bErrorCh[i];
            }

            //告知FW哪个通道出问题
            ReCode =Comm_Base_IIC_IO(bWBuf, (unsigned short)(iErrorNum + 2), NULL, 0 );
            for ( i = 0; i < 1; i++)
            {
            	  FTS_TEST_DBG("0x04 lwg iErrorNum= %d",iErrorNum);
            	  FTS_TEST_DBG("0x04 lwg %d",iErrorNum * (iErrorNum - 1) * 2);
                ReCode = WeakShort_GetAdcData(  iErrorNum * (iErrorNum - 1) * 2, iTmpAdcData, 0x04);
                SysDelay(50);

                if (ReCode != ERROR_CODE_OK)
                {
                    FTS_TEST_ERROR("\r\n\r\n// Failed to Get Weak short Data. Error Code: %d", ReCode);
                    btmpresult = false;
                    goto TEST_END;
                }
            }

#ifdef DEBUG_WEAKSHORT
            FTS_TEST_DBG("\r\nMutual AdcData:\r\n");
            for ( i=0; i<iErrorNum * (iErrorNum - 1); i++)
            {
                FTS_TEST_DBG(" 0x%x, ",*(iTmpAdcData+i));
            }
#endif

            ////计算
            iDrefn = iClbData_Mutual;
            iAdcCount = 0;

            //先把通道间的电阻值存到数据
            for ( i = 0; i < iErrorNum; i++)
            {
                for ( j = 0; j < iErrorNum; j++)
                {
                    if (bErrorCh[i] == bErrorCh[j]) continue;
                    iDsen = iTmpAdcData[iAdcCount];
                    iAdcCount++;
                    if (iDrefn - iDsen <= 0)//小于等于0，直接判PASS
                    {
                        continue;
                    }
                    fValue = (202 * (iDsen - iDoffset) + 79786) / (iDrefn - iDsen) - 3;
                    if (fValue < 0) fValue = 0;
                    strtCmpCh[iCmpChCount].bCh1 = bErrorCh[i];
                    strtCmpCh[iCmpChCount].bCh2 = bErrorCh[j];
                    strtCmpCh[iCmpChCount].fResistanceValue = fValue;
                    strtCmpCh[iCmpChCount].iAdcValue = iDsen;
                    iCmpChCount++;

                }
            }

            //从通道间的两个电阻值挑出最大的电阻值，作为此两通道间的电阻值，然后再判断是否小于预设阈值
            for ( i = 0; i < iCmpChCount; i++)
            {
                for (j = i + 1; j < iCmpChCount; j++)
                {
                    //找到此两个通道?
                    if ((strtCmpCh[i].bCh1 == strtCmpCh[j].bCh2) && (strtCmpCh[i].bCh2 == strtCmpCh[j].bCh1))
                    {
                        //比较值大小
                        fValue = strtCmpCh[i].fResistanceValue;
                        iValue = strtCmpCh[i].iAdcValue;
                        if (strtCmpCh[i].fResistanceValue < strtCmpCh[j].fResistanceValue)
                        {
                            fValue = strtCmpCh[j].fResistanceValue;
                            iValue = strtCmpCh[j].iAdcValue;
                        }

                        //是否小于预设阈值
                        if (iMin_CC > fValue)
                        {
                            strtErrorCh[iErrChCount].bCh1 = strtCmpCh[i].bCh1;
                            strtErrorCh[iErrChCount].bCh2 = strtCmpCh[i].bCh2;
                            strtErrorCh[iErrChCount].fResistanceValue = fValue;
                            strtErrorCh[iErrChCount].iAdcValue = iValue;

                            fMShortResistance[strtErrorCh[iErrChCount].bCh1 - 1] = fValue;//更新为复检之后的电阻值
                            fMShortResistance[strtErrorCh[iErrChCount].bCh2 - 1] = fValue;//更新为复检之后的电阻值
                            iAdcData[strtErrorCh[iErrChCount].bCh1 + 2 + iChannelNum] = iValue;//更新为复检之后的ADC值
                            iAdcData[strtErrorCh[iErrChCount].bCh2 + 2 + iChannelNum] = iValue;//更新为复检之后的ADC值

                            if (strtErrorCh[iErrChCount].fResistanceValue > 70)
                            {
                                //打印互短通道信息
                                if (strtErrorCh[iErrChCount].bCh1 <= iTxNum)
                                {

                                    FTS_TEST_DBG("Tx%d", (strtErrorCh[iErrChCount].bCh1) );
                                }
                                else
                                {

                                    FTS_TEST_DBG( "Rx%d", (strtErrorCh[iErrChCount].bCh1 - iTxNum) );
                                }

                                if (strtErrorCh[iErrChCount].bCh2 <= iTxNum)
                                {
                                    FTS_TEST_DBG("Tx%d", (strtErrorCh[iErrChCount].bCh2) );
                                }
                                else
                                {

                                    FTS_TEST_DBG( "Rx%d", (strtErrorCh[iErrChCount].bCh2 - iTxNum) );
                                }

                                FTS_TEST_DBG(": %.02d(kΩ), ADC: %d", strtErrorCh[iErrChCount].fResistanceValue, strtErrorCh[iErrChCount].iAdcValue);
                                btmpresult = false;
                                bPrintMsg = true;
                            }

                            iErrChCount++;

                        }

                    }
                }

            }

            //如果互短有小于70K的，存下来，再次判断
            for ( i = 0; i < iErrChCount; i++)
            {
                if (strtErrorCh[i].fResistanceValue < 70)
                {
                    iNowMin70kNum = iMin70kNum;
                    bIsUsed = false;
                    for ( iNum = 0; iNum < iNowMin70kNum; iNum++)
                    {
                        if (strtErrorCh[i].bCh1 == bMin70kCh[iNum])
                        {
                            bIsUsed = true;
                            break;
                        }
                    }
                    if (!bIsUsed)
                    {
                        bMin70kCh[iMin70kNum] = strtErrorCh[i].bCh1;
                        iMin70kNum++;
                    }
                    bIsUsed = false;
                    for ( iNum = 0; iNum < iNowMin70kNum; iNum++)
                    {
                        if (strtErrorCh[i].bCh2 == bMin70kCh[iNum])
                        {
                            bIsUsed = true;
                            break;
                        }
                    }

                    if (!bIsUsed)
                    {
                        bMin70kCh[iMin70kNum] = strtErrorCh[i].bCh2;
                        iMin70kNum++;
                    }
                }
            }

            if (strtCmpCh != NULL)
            {
                fts_free(strtCmpCh);
                strtCmpCh = NULL;
            }
            if (strtErrorCh != NULL)
            {
                fts_free(strtErrorCh);
                strtErrorCh = NULL;
            }

        }

        //存在小于70K的，再次检测互短
        if (iMin70kNum > 0)//出现电阻值小于70K的通道
        {

            iCmpChCount = 0;
            iErrChCount = 0;
            strtCmpCh = fts_malloc((iMin70kNum * (iMin70kNum - 1))*sizeof(struct structCompareChannel));
            strtErrorCh = fts_malloc((iMin70kNum * (iMin70kNum - 1) / 2)*sizeof(struct structCompareChannel));


            bWBuf[0] = 0xF8;
            bWBuf[1] = (unsigned char)iMin70kNum;
            for (i = 0; i < iMin70kNum; i++)
            {
                bWBuf[2 + i] = bMin70kCh[i];
            }
            //告知FW哪个通道出问题
            ReCode = Comm_Base_IIC_IO( bWBuf, (unsigned short)(iMin70kNum + 2), NULL, 0 );
            for ( i = 0; i < 1; i++)
            {
            	  FTS_TEST_DBG("0x05 iMin70kNum =  %d",iMin70kNum);
                ReCode =WeakShort_GetAdcData(iMin70kNum *(iMin70kNum - 1) * 2, iTmpAdcData, 0x05 );
                SysDelay(50);
                if (ReCode != ERROR_CODE_OK)
                {
                    FTS_TEST_ERROR("\r\n\r\n// Failed to Get Weak short Data. Error Code: %d", ReCode);
                    btmpresult = false;
                    goto TEST_END;
                }
            }

#ifdef DEBUG_WEAKSHORT
            FTS_TEST_DBG("\r\nMutual AdcData_Min70K:\r\n");
            for ( i=0; i<iMin70kNum * (iMin70kNum - 1); i++)
            {
                FTS_TEST_DBG(" 0x%x, ",*(iTmpAdcData+i));
            }
#endif
            iDrefn = iClbData_Mutual;
            iAdcCount = 0;

            //先把通道间的电阻值存到数据
            for ( i = 0; i < iMin70kNum; i++)
            {
                for ( j = 0; j < iMin70kNum; j++)
                {
                    if (bErrorCh[i] == bErrorCh[j]) continue;
                    iDsen = iTmpAdcData[iAdcCount];
                    iAdcCount++;
                    if (iDrefn - iDsen <= 0)//小于等于0，直接判PASS
                    {
                        continue;
                    }
                    fValue =(4* (iDsen - iDoffset) + 1698) / (iDrefn - iDsen) - 3;
                    if (fValue < 0) fValue = 0;
                    strtCmpCh[iCmpChCount].bCh1 = bErrorCh[i];
                    strtCmpCh[iCmpChCount].bCh2 = bErrorCh[j];
                    strtCmpCh[iCmpChCount].fResistanceValue = fValue;
                    strtCmpCh[iCmpChCount].iAdcValue = iDsen;
                    iCmpChCount++;
                }
            }

            //从通道间的两个电阻值挑出最大的电阻值，作为此两通道间的电阻值，然后再判断是否小于预设阈值
            for ( i = 0; i < iCmpChCount; i++)
            {
                for ( j = i; j < iCmpChCount; j++)
                {
                    //找到此两个通道，
                    if ((strtCmpCh[i].bCh1 == strtCmpCh[j].bCh2) && (strtCmpCh[i].bCh2 == strtCmpCh[j].bCh1))
                    {
                        //比较值大小
                        fValue = strtCmpCh[i].fResistanceValue;
                        iValue = strtCmpCh[i].iAdcValue;
                        if (strtCmpCh[i].fResistanceValue < strtCmpCh[j].fResistanceValue)
                        {
                            fValue = strtCmpCh[j].fResistanceValue;
                            iValue = strtCmpCh[j].iAdcValue;
                        }

                        //是否小于预设阈值
                        if (iMin_CC > fValue)
                        {
                            strtErrorCh[iErrChCount].bCh1 = strtCmpCh[i].bCh1;
                            strtErrorCh[iErrChCount].bCh2 = strtCmpCh[i].bCh2;
                            strtErrorCh[iErrChCount].fResistanceValue = fValue;
                            strtErrorCh[iErrChCount].iAdcValue = iValue;

                            fMShortResistance[strtErrorCh[iErrChCount].bCh1 - 1] = fValue;//更新为复检之后的电阻值
                            fMShortResistance[strtErrorCh[iErrChCount].bCh2 - 1] = fValue;//更新为复检之后的电阻值
                            iAdcData[strtErrorCh[iErrChCount].bCh1 + 2 + iChannelNum] = iValue;//更新为复检之后的ADC值
                            iAdcData[strtErrorCh[iErrChCount].bCh2 + 2 + iChannelNum] = iValue;//更新为复检之后的ADC值

                            //打印互短通道信息
                            if (strtErrorCh[iErrChCount].bCh1 <= iTxNum)
                            {
                                FTS_TEST_DBG("Tx%d", (strtErrorCh[iErrChCount].bCh1) );
                            }
                            else
                            {

                                FTS_TEST_DBG( "Rx%d", (strtErrorCh[iErrChCount].bCh1 - iTxNum) );
                            }

                            if (strtErrorCh[iErrChCount].bCh2 <= iTxNum)
                            {

                                FTS_TEST_DBG("Tx%d", (strtErrorCh[iErrChCount].bCh2) );
                            }
                            else
                            {

                                FTS_TEST_DBG( "Rx%d", (strtErrorCh[iErrChCount].bCh2 - iTxNum) );
                            }

                            FTS_TEST_DBG(": %.02d(kΩ), ADC: %d", strtErrorCh[iErrChCount].fResistanceValue, strtErrorCh[iErrChCount].iAdcValue);
                            iErrChCount++;
                            btmpresult = false;
                            bPrintMsg = true;
                        }
                    }
                }
            }

            if (strtCmpCh != NULL)
            {
                fts_free(strtCmpCh);
                strtCmpCh = NULL;
            }
            if (strtErrorCh != NULL)
            {
                fts_free(strtErrorCh);
                strtErrorCh = NULL;
            }
        }

        if (bErrorCh != NULL)
        {
            fts_free (bErrorCh);
            bErrorCh = NULL;
        }
        if (bMin70kCh != NULL)
        {
            fts_free (bMin70kCh);
            bMin70kCh = NULL;
        }
    }

#if 1
    //释放数组空间
    if (bWBuf != NULL)
    {
        fts_free(bWBuf);
        bWBuf = NULL;
    }
    if (iAdcData != NULL)
    {
        fts_free(iAdcData);
        iAdcData = NULL;
    }
    if (iTmpAdcData != NULL)
    {
        fts_free(iTmpAdcData);
        iTmpAdcData = NULL;
    }
    if (fMShortResistance != NULL)
    {
        fts_free(fMShortResistance);
        fMShortResistance = NULL;
    }
    if (fGShortResistance != NULL)
    {
        fts_free(fGShortResistance);
        fGShortResistance = NULL;
    }
    if (fOriginResistance != NULL)
    {
        fts_free(fOriginResistance);
        fOriginResistance = NULL;
    }
#endif


TEST_END:

    FTS_TEST_DBG(" ClbData_GND:%d, ClbData_Mutual:%d, Offset:%d",  iClbData_Ground, iClbData_Mutual, iOffset);
    if (bIsWeakShortGND && bIsWeakShortMut)
    {
        FTS_TEST_DBG("GND and Mutual Weak Short!");
    }
    else if (bIsWeakShortGND)
    {
        FTS_TEST_DBG("GND Weak Short!");
    }
    else if (bIsWeakShortMut)
    {
        FTS_TEST_DBG("Mutual Weak Short!");
    }
    else
    {
        FTS_TEST_DBG("No Short!");
    }


    if ( btmpresult )
    {

        * bTestResult = true;
    }
    else
    {

        * bTestResult = false;
    }

    return ReCode;

}

unsigned char WeakShort_GetGndClbData(int *iValue)
{
    unsigned char ReCode = ERROR_CODE_OK;
    unsigned char *bWBuf = NULL;
    unsigned char *bRBuf = NULL;
    unsigned char Data = 0xff;
    int i = 0;

    bWBuf = fts_malloc(128*sizeof(unsigned char));
    bRBuf = fts_malloc(512*sizeof(unsigned char));

    memset( bWBuf, 0, 128 );
    memset( bRBuf, 0, 512 );

    bWBuf[0] = 0xF8;
    bWBuf[1] = 1;//异常个数
    bWBuf[2] = 0;//发0通道，即是读出对地校准ADC
    ReCode = Comm_Base_IIC_IO(bWBuf, 3, NULL, 0);//告知FW哪个通道出问题
    SysDelay(50);

    ReCode = WriteReg(0x07, 0x02);//对地短电阻大于70K的通道进行第二次检测，使能命令：W: 07 02
    for ( i = 0; i < 40; i++)//准备好数据后，FW将0x07寄存器置为0
    {
        SysDelay(50);
        ReCode = ReadReg(0x07, &Data);
        if (ReCode == ERROR_CODE_OK)
        {
            if (Data == 0) break;
        }
    }

    bRBuf[0] = 0xF4;
    ReCode = Comm_Base_IIC_IO( bRBuf, 1, bRBuf + 1, 2);
    if (ReCode == ERROR_CODE_OK)
    {
        *iValue = (bRBuf[1] << 8) + bRBuf[2];
    }

    if (bRBuf != NULL)
    {
        fts_free(bRBuf);
        bRBuf = NULL;
    }

    if (bWBuf != NULL)
    {
        fts_free(bWBuf);
        bWBuf = NULL;
    }

    return ReCode;

}

static unsigned char WeakShort_GetAdcData( int AllAdcDataLen, int *pRevBuffer, unsigned char chCmd  )
{
    unsigned char ReCode = ERROR_CODE_OK;
    int i = 0;
    unsigned char Data = 0xff;
    unsigned char *pDataSend = NULL;
    int iReadDataLen = AllAdcDataLen;//Offset*2 + (ClbData + TxNum + RxNum)*2*2

    FTS_TEST_FUNC_ENTER();

    pDataSend = fts_malloc(iReadDataLen + 2);
    if (pDataSend == NULL)   return ERROR_CODE_ALLOCATE_BUFFER_ERROR;
    memset( pDataSend, 0, iReadDataLen + 2 );

    ReCode = WriteReg(0x07, chCmd);// 主机发送使能后微短路测试一次
    if (ReCode != ERROR_CODE_OK)return ReCode;

    SysDelay(20);
    for ( i = 0; i < 40; i++) //准备好数据后，FW将0x07寄存器置为0
    {
        SysDelay(10+i*3);
        ReCode = ReadReg(0x07, &Data);
        if (ReCode == ERROR_CODE_OK)
        {
            FTS_TEST_DBG("Data(%d) = %d,", (i+1), Data);
            if (Data == 0)
            {
                break;
            }

        }
    }
 
    pDataSend[0] = 0xF4;
    ReCode = Comm_Base_IIC_IO(pDataSend, 1, pDataSend + 1, iReadDataLen);
    if (ReCode == ERROR_CODE_OK)
    {
        FTS_TEST_DBG("\n Adc Data:\n");
        for (i = 0; i < iReadDataLen/2; i++)
        {
            pRevBuffer[i] = (pDataSend[1 + 2*i]<<8) + pDataSend[1 + 2*i + 1];
            //FTS_TEST_DBG("%d,    ", pRevBuffer[i]);   //  FTS_TEST_DBG("pRevBuffer[%d]:%d,    ", i, pRevBuffer[i]);
        }
        FTS_TEST_DBG("\n");
    }
    else
    {
        FTS_TEST_ERROR("Comm_Base_IIC_IO error. error:%d. \n", ReCode);
    }


    if (pDataSend != NULL)
    {
        fts_free(pDataSend);
        pDataSend = NULL;
    }

    FTS_TEST_FUNC_EXIT();

    return ReCode;
}

/*add by lmh end*/

/************************************************************************
* Name: GetPanelRows(Same function name as FT_MultipleTest)
* Brief:  Get row of TP
* Input: none
* Output: pPanelRows
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetPanelRows(unsigned char *pPanelRows)
{
    return ReadReg(REG_TX_NUM, pPanelRows);
}

/************************************************************************
* Name: GetPanelCols(Same function name as FT_MultipleTest)
* Brief:  get column of TP
* Input: none
* Output: pPanelCols
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetPanelCols(unsigned char *pPanelCols)
{
    return ReadReg(REG_RX_NUM, pPanelCols);
}
/************************************************************************
* Name: StartScan(Same function name as FT_MultipleTest)
* Brief:  Scan TP, do it before read Raw Data
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int StartScan(void)
{
    unsigned char RegVal = 0;
    unsigned char times = 0;
    const unsigned char MaxTimes = 250;  //The longest wait 160ms
    unsigned char ReCode = ERROR_CODE_COMM_ERROR;

    ReCode = ReadReg(DEVIDE_MODE_ADDR, &RegVal);
    if (ReCode == ERROR_CODE_OK)
    {
        RegVal |= 0x80;     //Top bit position 1, start scan
        ReCode = WriteReg(DEVIDE_MODE_ADDR, RegVal);
        if (ReCode == ERROR_CODE_OK)
        {
            while (times++ < MaxTimes)      //Wait for the scan to complete
            {
                SysDelay(8);    //8ms
                ReCode = ReadReg(DEVIDE_MODE_ADDR, &RegVal);
                if (ReCode == ERROR_CODE_OK)
                {
                    if ((RegVal>>7) == 0)    break;
                }
                else
                {
                    break;
                }
            }
            if (times < MaxTimes)    ReCode = ERROR_CODE_OK;
            else ReCode = ERROR_CODE_COMM_ERROR;
        }
    }
    return ReCode;

}
/************************************************************************
* Name: ReadRawData(Same function name as FT_MultipleTest)
* Brief:  read Raw Data
* Input: Freq(No longer used, reserved), LineNum, ByteNum
* Output: pRevBuffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char ReadRawData(unsigned char Freq, unsigned char LineNum, int ByteNum, int *pRevBuffer)
{
    unsigned char ReCode=ERROR_CODE_COMM_ERROR;
    unsigned char I2C_wBuffer[3];
    int i, iReadNum;
    unsigned short BytesNumInTestMode1=0;



    iReadNum=ByteNum/BYTES_PER_TIME;

    if (0 != (ByteNum%BYTES_PER_TIME)) iReadNum++;

    if (ByteNum <= BYTES_PER_TIME)
    {
        BytesNumInTestMode1 = ByteNum;
    }
    else
    {
        BytesNumInTestMode1 = BYTES_PER_TIME;
    }

    ReCode = WriteReg(REG_LINE_NUM, LineNum);//Set row addr;


    //***********************************************************Read raw data
    I2C_wBuffer[0] = REG_RawBuf0;   //set begin address
    if (ReCode == ERROR_CODE_OK)
    {
        focal_msleep(10);
        ReCode = Comm_Base_IIC_IO(I2C_wBuffer, 1, m_ucTempData, BytesNumInTestMode1);
    }

    for (i=1; i<iReadNum; i++)
    {
        if (ReCode != ERROR_CODE_OK) break;

        if (i==iReadNum-1) //last packet
        {
            focal_msleep(10);
            ReCode = Comm_Base_IIC_IO(NULL, 0, m_ucTempData+BYTES_PER_TIME*i, ByteNum-BYTES_PER_TIME*i);
        }
        else
        {
            focal_msleep(10);
            ReCode = Comm_Base_IIC_IO(NULL, 0, m_ucTempData+BYTES_PER_TIME*i, BYTES_PER_TIME);
        }

    }

    if (ReCode == ERROR_CODE_OK)
    {
        for (i=0; i<(ByteNum>>1); i++)
        {
            pRevBuffer[i] = (m_ucTempData[i<<1]<<8)+m_ucTempData[(i<<1)+1];
            //if(pRevBuffer[i] & 0x8000)//have sign bit
            //{
            //  pRevBuffer[i] -= 0xffff + 1;
            //}
        }
    }

    return ReCode;

}
/************************************************************************
* Name: GetTxSC_CB(Same function name as FT_MultipleTest)
* Brief:  get CB of Tx SCap
* Input: index
* Output: pcbValue
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char GetTxSC_CB(unsigned char index, unsigned char *pcbValue)
{
    unsigned char ReCode = ERROR_CODE_OK;
    unsigned char wBuffer[4];

    if (index<128) //single read
    {
        *pcbValue = 0;
        WriteReg(REG_ScCbAddrR, index);
        ReCode = ReadReg(REG_ScCbBuf0, pcbValue);
    }
    else//Sequential Read length index-128
    {
        WriteReg(REG_ScCbAddrR, 0);
        wBuffer[0] = REG_ScCbBuf0;
        ReCode = Comm_Base_IIC_IO(wBuffer, 1, pcbValue, index-128);

    }

    return ReCode;
}


/************************************************************************
* Name: Save_Test_Data
* Brief:  Storage format of test data
* Input: int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount
* Output: none
* Return: none
***********************************************************************/
static void Save_Test_Data(int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount)
{
    int iLen = 0;
    int i = 0, j = 0;

    //Save  Msg (ItemCode is enough, ItemName is not necessary, so set it to "NA".)
    iLen= sprintf(g_pTmpBuff,"NA, %d, %d, %d, %d, %d, ", \
                  m_ucTestItemCode, Row, Col, m_iStartLine, ItemCount);
    memcpy(g_pMsgAreaLine2+g_lenMsgAreaLine2, g_pTmpBuff, iLen);
    g_lenMsgAreaLine2 += iLen;

    m_iStartLine += Row;
    m_iTestDataCount++;

    //Save Data
    for (i = 0+iArrayIndex; i < Row+iArrayIndex; i++)
    {
        for (j = 0; j < Col; j++)
        {
            if (j == (Col -1)) //The Last Data of the Row, add "\n"
                iLen= sprintf(g_pTmpBuff,"%d, \n ",  iData[i][j]);
            else
                iLen= sprintf(g_pTmpBuff,"%d, ", iData[i][j]);

            memcpy(g_pStoreDataArea+g_lenStoreDataArea, g_pTmpBuff, iLen);
            g_lenStoreDataArea += iLen;
        }
    }

}

/************************************************************************
* Name: GetChannelNum
* Brief:  Get Channel Num(Tx and Rx)
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetChannelNum(void)
{
    unsigned char ReCode;
    unsigned char rBuffer[1]; //= new unsigned char;

    //m_strCurrentTestMsg = "Get Tx Num...";
    ReCode = GetPanelRows(rBuffer);
    if (ReCode == ERROR_CODE_OK)
    {
        g_ScreenSetParam.iTxNum = rBuffer[0];
        if (g_ScreenSetParam.iTxNum+4 > g_ScreenSetParam.iUsedMaxTxNum)
        {
            FTS_TEST_ERROR("Failed to get Tx number, Get num = %d, UsedMaxNum = %d",
                           g_ScreenSetParam.iTxNum, g_ScreenSetParam.iUsedMaxTxNum);
            g_ScreenSetParam.iTxNum = 0;
            return ERROR_CODE_INVALID_PARAM;
        }
//g_ScreenSetParam.iTxNum = 26;
    }
    else
    {
        FTS_TEST_ERROR("Failed to get Tx number");
    }

    ///////////////m_strCurrentTestMsg = "Get Rx Num...";

    ReCode = GetPanelCols(rBuffer);
    if (ReCode == ERROR_CODE_OK)
    {
        g_ScreenSetParam.iRxNum = rBuffer[0];
        if (g_ScreenSetParam.iRxNum > g_ScreenSetParam.iUsedMaxRxNum)
        {
            FTS_TEST_ERROR("Failed to get Rx number, Get num = %d, UsedMaxNum = %d",
                           g_ScreenSetParam.iRxNum, g_ScreenSetParam.iUsedMaxRxNum);
            g_ScreenSetParam.iRxNum = 0;
            return ERROR_CODE_INVALID_PARAM;
        }
        //g_ScreenSetParam.iRxNum = 28;
    }
    else
    {
        FTS_TEST_ERROR("Failed to get Rx number");
    }

    return ReCode;

}
/************************************************************************
* Name: GetRawData
* Brief:  Get Raw Data of MCAP
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetRawData(void)
{
    unsigned char ReCode = ERROR_CODE_OK;
    int iRow = 0;
    int iCol = 0;

    //--------------------------------------------Enter Factory Mode
    ReCode = EnterFactory();
    if ( ERROR_CODE_OK != ReCode )
    {
        FTS_TEST_ERROR("Failed to Enter Factory Mode...");
        return ReCode;
    }


    //--------------------------------------------Check Num of Channel
    if (0 == (g_ScreenSetParam.iTxNum + g_ScreenSetParam.iRxNum))
    {
        ReCode = GetChannelNum();
        if ( ERROR_CODE_OK != ReCode )
        {
            FTS_TEST_ERROR("Error Channel Num...");
            return ERROR_CODE_INVALID_PARAM;
        }
    }

    //--------------------------------------------Start Scanning
    FTS_TEST_DBG("Start Scan ...");
    ReCode = StartScan();
    if (ERROR_CODE_OK != ReCode)
    {
        FTS_TEST_ERROR("Failed to Scan ...");
        return ReCode;
    }

    //--------------------------------------------Read RawData, Only MCAP
    memset(m_RawData, 0, sizeof(m_RawData));
    ReCode = ReadRawData( 1, 0xAA, ( g_ScreenSetParam.iTxNum * g_ScreenSetParam.iRxNum )*2, m_iTempRawData );
    for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
    {
        for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
        {
            m_RawData[iRow][iCol] = m_iTempRawData[iRow*g_ScreenSetParam.iRxNum + iCol];
        }
    }
    return ReCode;
}
/************************************************************************
* Name: ShowRawData
* Brief:  Show RawData
* Input: none
* Output: none
* Return: none.
***********************************************************************/
static void ShowRawData(void)
{
    int iRow, iCol;
    //----------------------------------------------------------Show RawData
    for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
    {
        FTS_TEST_DBG("Tx%2d:  ", iRow+1);
        for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
        {
            FTS_TEST_DBG("%5d    ", m_RawData[iRow][iCol]);
        }
        FTS_TEST_DBG("\n ");
    }
}

/************************************************************************
* Name: GetChannelNumNoMapping
* Brief:  get Tx&Rx num from other Register
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetChannelNumNoMapping(void)
{
    unsigned char ReCode;
    unsigned char rBuffer[1]; //= new unsigned char;


    FTS_TEST_DBG("Get Tx Num...");
    ReCode =ReadReg( REG_TX_NOMAPPING_NUM,  rBuffer);
    if (ReCode == ERROR_CODE_OK)
    {
        g_ScreenSetParam.iTxNum= rBuffer[0];
    }
    else
    {
        FTS_TEST_ERROR("Failed to get Tx number");
    }


    FTS_TEST_DBG("Get Rx Num...");
    ReCode = ReadReg( REG_RX_NOMAPPING_NUM,  rBuffer);
    if (ReCode == ERROR_CODE_OK)
    {
        g_ScreenSetParam.iRxNum = rBuffer[0];
    }
    else
    {
        FTS_TEST_ERROR("Failed to get Rx number");
    }

    return ReCode;
}
/************************************************************************
* Name: SwitchToNoMapping
* Brief:  If it is V3 pattern, Get Tx/Rx Num again
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char SwitchToNoMapping(void)
{
    unsigned char chPattern = -1;
    unsigned char ReCode = ERROR_CODE_OK;
    unsigned char RegData = -1;
    ReCode = ReadReg( REG_PATTERN_5422, &chPattern );//

    if (1 == chPattern) // 1: V3 Pattern
    {
        RegData = -1;
        ReCode =ReadReg( REG_MAPPING_SWITCH, &RegData );
        if ( 1 != RegData )
        {
            ReCode = WriteReg( REG_MAPPING_SWITCH, 1 );  //0-mapping 1-no mampping
            focal_msleep(20);
            GetChannelNumNoMapping();
        }
    }

    if ( ReCode != ERROR_CODE_OK )
    {
        FTS_TEST_ERROR("Switch To NoMapping Failed!");
    }
    return ReCode;
}
/************************************************************************
* Name: GetTestCondition
* Brief:  Check whether Rx or TX need to test, in Waterproof ON/OFF Mode.
* Input: none
* Output: none
* Return: true: need to test; false: Not tested.
***********************************************************************/
static boolean GetTestCondition(int iTestType, unsigned char ucChannelValue)
{
    boolean bIsNeeded = false;
    switch (iTestType)
    {
        case WT_NeedProofOnTest://Bit5:  0:test waterProof mode ;  1 not test waterProof mode
            bIsNeeded = !( ucChannelValue & 0x20 );
            break;
        case WT_NeedProofOffTest://Bit7: 0: test normal mode  1:not test normal mode
            bIsNeeded = !( ucChannelValue & 0x80 );
            break;
        case WT_NeedTxOnVal:
            //Bit6:  0 : test waterProof Rx+Tx  1:test waterProof single channel
            //Bit2:  0: test waterProof Tx only;  1:  test waterProof Rx only
            bIsNeeded = !( ucChannelValue & 0x40 ) || !( ucChannelValue & 0x04 );
            break;
        case WT_NeedRxOnVal:
            //Bit6:  0 : test waterProof Rx+Tx  1 test waterProof single channel
            //Bit2:  0: test waterProof Tx only;  1:  test waterProof Rx only
            bIsNeeded = !( ucChannelValue & 0x40 ) || ( ucChannelValue & 0x04 );
            break;
        case WT_NeedTxOffVal://Bit1,Bit0:  00:test normal Tx; 10: test normal Rx+Tx
            bIsNeeded = (0x00 == (ucChannelValue & 0x03)) || (0x02 == ( ucChannelValue & 0x03 ));
            break;
        case WT_NeedRxOffVal://Bit1,Bit0:  01: test normal Rx;    10: test normal Rx+Tx
            bIsNeeded = (0x01 == (ucChannelValue & 0x03)) || (0x02 == ( ucChannelValue & 0x03 ));
            break;
        default:
            break;
    }
    return bIsNeeded;
}

#endif


