#include "CSelfCalibration2Controller.h"
#include <sstream>
#include <bitset>
#include <memory>
#define CHECK_PERIOD_IN_MS 100
/* RECTIFY  */
#define N_RECTIFY_PAYLOAD_STEP_BYTES (3)
#define N_RECTIFY_BUFFER_PREFIX_BYTES (2)
#define REG_ADDR_RECTIFY (0xf5)
#define _COUT_FUNCTION_CALL_LOG_ false
#define _CALIB_HW_API_DEBUG_ false
#define HWAPI_RECTIFICATION_N_DATA_STRUCTURE_ELEMENTS (76)
/* RECTIFICATION DATA STRUCTURE INFORMATION */
// ******************************************************************************
static const short Rectification_Data_Structure_Bytes_Offset[HWAPI_RECTIFICATION_N_DATA_STRUCTURE_ELEMENTS] = {
        0, 1,  2, 4, 6, 8, 10, 13, 15, 17, 20, 22, 24, 27, 29, 31, 33, 35, 37, 39, 42, 44, 46, 49, 51, 53, 56, 58, 60, 62, 64, 65, 66, 67, 68, 70, 72, 74, 76, 78, 80, 82, 84, 87, 90, 91, 92, 94, 96, 98, 100, 102, 104, 106, 108, 111, 114, 115, 117, 119, 121, 123, 125, 127, 128, 144, 160, 175, 176, 186, 188, 190, 191, 193, 194, 196 };
static const short Rectification_Data_Structure_Size_in_Bytes[HWAPI_RECTIFICATION_N_DATA_STRUCTURE_ELEMENTS] = {
        1, 1, 2, 2, 2, 2, 3, 2, 2, 3, 2, 2, 3, 2, 2, 2, 2, 2, 2, 3, 2, 2, 3, 2, 2, 3, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 1, 2, 2, 2, 2, 2, 2, 1, 16, 16, 15, 1, 10, 2, 2, 2, 2, 2, 2, 2 };
static const short Rectification_Data_Structure_Size_in_Bits[HWAPI_RECTIFICATION_N_DATA_STRUCTURE_ELEMENTS] = {
        8, 8, 12, 12, 16, 11, 19, 11, 16, 19, 12, 10, 19, 10, 11, 10, 11, 16, 11, 19, 11, 16, 19, 12, 10, 19, 10, 11, 10, 11, 8, 8, 7, 8, 16, 16, 16, 16, 16, 16, 16, 16, 18, 18, 8, 7, 16, 16, 16, 16, 16, 16, 16, 16, 18, 18, 8, 13, 13, 11, 11, 11, 11, 8, 128, 128, 120, 7, 79, 4, 4, 2, 6, 2, 6, 15 };
//static const short Rectification_Data_Structure_Local_Bits_Offset[HWAPI_RECTIFICATION_N_DATA_STRUCTURE_ELEMENTS] = { 0, 0, 1, 1, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static const short Rectification_Data_Structure_Fractional_Bits[HWAPI_RECTIFICATION_N_DATA_STRUCTURE_ELEMENTS] = {
        0, 0, 0, 0, 21, 21, 17, 21, 21, 17, 21, 21, 17, 0, -1, 0, -1, 21, 21, 17, 21, 21, 17, 21, 21, 17, 0, -1, 0, -1, 13, 0, 13, 0, 10, 10, 10, 10, 10, 10, 2, 2, 3, 3, 13, 13, 10, 10, 10, 10, 10, 10, 2, 2, 3, 3, 0, 9, 9, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 9, 9, -1, -1, -1, -1, 0 };
static const short Rectification_Data_Structure_Little_Endian[HWAPI_RECTIFICATION_N_DATA_STRUCTURE_ELEMENTS] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1 };
static const short Rectification_Data_Structure_DontCare[HWAPI_RECTIFICATION_N_DATA_STRUCTURE_ELEMENTS] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1 };
static const short Rectification_Data_Structure_HighByte_MSB_Index[HWAPI_RECTIFICATION_N_DATA_STRUCTURE_ELEMENTS] = {
        7, 7, 3, 3, 7, 2, 2, 2, 7, 2, 3, 1, 2, 1, 2, 1, 2, 7, 2, 2, 2, 7, 2, 3, 1, 2, 1, 2, 1, 2, 7, 7, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 1, 1, 7, 6, 7, 7, 7, 7, 7, 7, 7, 7, 1, 1, 7, 4, 4, 2, 2, 2, 2, 7, 127, 127, 119, 7, 79, 4, 4, 2, 6, 2, 6, 15 };
static const short Rectification_Data_Structure_HighByte_LSB_Index[HWAPI_RECTIFICATION_N_DATA_STRUCTURE_ELEMENTS] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 4, 0 };
// ******************************************************************************

static const unsigned short _THERMAL_SENSOR_SLAVE_ADDR_80362 = (0x30);
static const unsigned short _THERMAL_SENSOR_SLAVE_ADDR_8083 = (0x20);
static const unsigned short _THERMAL_SENSOR_SLAVE_ADDR_AR0135 = (0x30);

enum class E_THERMAL_SENSOR_MODEL {
    ONSEMI_AR0135 = 100,
    ONSEMI_AR0144 = 101,

    STM_VD5X = 200,

    OV_OG02 = 300,
    OV_OG01 = 301,

    TI_TMP1075 = 1000,
};


#define _ONSEMI_AR0144_PRESET_ADDR (0x30B4)
#define _ONSEMI_AR0144_PRESET_VALUE (0x0011)
#define _ONSEMI_AR0144_CALIB_DATA_READ_ADDR (0x30C6)
#define _ONSEMI_AR0144_CALIB_DATA_TEMP (55)
#define _ONSEMI_AR0144_TEMP_CURVE_SLOPE (0.7)
#define _ONSEMI_AR0144_TEMP_READ_ADDR (0x30B2)

#define _ONSEMI_AR0135_PRESET_ADDR (0x30B4)
#define _ONSEMI_AR0135_PRESET_VALUE (0x0011)
#define _ONSEMI_AR0135_CALIB_DATA_READ_ADDR (0x30C8)
#define _ONSEMI_AR0135_CALIB_DATA_TEMP (55)
#define _ONSEMI_AR0135_TEMP_CURVE_SLOPE (0.7)
#define _ONSEMI_AR0135_TEMP_READ_ADDR (0x30B2)

#define _STM_VD5X_TEMP_READ_ADDR (0x004C)

#define _1st_ (0) // can be left or top camera
#define _2nd_ (1) // can be right or bottom camera

// Global variables for buffer operations, save here first, after verifying the behavior change to class member scope.
int n_read_buffer_data_items = 0;
int n_write_buffer_data_items = 0;
int n_data_structure_elements = HWAPI_RECTIFICATION_N_DATA_STRUCTURE_ELEMENTS;
uint16_t table_size = 0;
CSelfCalibration2Controller::CFile_IO_Data_Structure_Rectify data_structure_scaled;
CSelfCalibration2Controller::CFile_IO_Data_Structure_Rectify data_structure;

int CSelfCalibration2Controller::get_temperature_param(uint16_t pid, E_THERMAL_SENSOR_MODEL& e_sensor_model,
                                                       int& sensor_slave_addr, int& nSensorMode) {
    switch (pid) {
        case APC_PID_80362:
        case APC_PID_IRIS:
            e_sensor_model = E_THERMAL_SENSOR_MODEL::ONSEMI_AR0144;
            sensor_slave_addr = _THERMAL_SENSOR_SLAVE_ADDR_80362;
            nSensorMode = SENSOR_A;
            break;
        case APC_PID_IVY2:
            e_sensor_model = E_THERMAL_SENSOR_MODEL::STM_VD5X;
            sensor_slave_addr = _THERMAL_SENSOR_SLAVE_ADDR_8083;
            nSensorMode = SENSOR_A;
            break;
        case APC_PID_8062:
            e_sensor_model = E_THERMAL_SENSOR_MODEL::ONSEMI_AR0135;
            sensor_slave_addr = _THERMAL_SENSOR_SLAVE_ADDR_AR0135;
            nSensorMode = SENSOR_A;
            break;
        default:
            fprintf(stderr, "Unsupported module get_temperature_param\n");
            break;
    }
    return 0;
}

/**
 * TODO: Move these function to each subclass of CVideoDeviceModel when schedule is ok. Put together first.
 */
int CSelfCalibration2Controller::get_temperature_of_img_sensor_ar0144(void* pHandleAPC, E_THERMAL_SENSOR_MODEL e_sensor_model, int sensor_slave_addr, int nSensorMode, float& fTemperature) {
    // change for linux version by Brook at 20230913
    int ret = 0;
    uint16_t QRegValue;
#ifdef WIN32
    if (!selfCalibration2Dlg->m_isSensorSlave)
        ret = APC_SetSensorRegister(selfCalibration2Dlg->m_hApcDI, &selfCalibration2Dlg->m_DevSelInfo, sensor_slave_addr, _ONSEMI_AR0144_PRESET_ADDR, _ONSEMI_AR0144_PRESET_VALUE, FG_Address_2Byte | FG_Value_2Byte, nSensorMode);
    else
        ret = APC_SetSlaveSensorRegister(selfCalibration2Dlg->m_hApcDI, &selfCalibration2Dlg->m_DevSelInfo, sensor_slave_addr, _ONSEMI_AR0144_PRESET_ADDR, _ONSEMI_AR0144_PRESET_VALUE, FG_Address_2Byte | FG_Value_2Byte, nSensorMode);
#elif __linux__
    DEVSELINFO *devSelInfo = m_deviceSelInfoList[0];
    ret = APC_SetSensorRegister(pHandleAPC, devSelInfo, sensor_slave_addr, _ONSEMI_AR0144_PRESET_ADDR,
                                _ONSEMI_AR0144_PRESET_VALUE, FG_Address_2Byte | FG_Value_2Byte, (SENSORMODE_INFO) nSensorMode);

#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if (ret != APC_OK) {
        std::cout << "APC_SetSensorRegister Failed." << std::endl;
        return -1;
    }
#ifdef WIN32
    if (!selfCalibration2Dlg->m_isSensorSlave)
        ret = APC_GetSensorRegister(selfCalibration2Dlg->m_hApcDI, &selfCalibration2Dlg->m_DevSelInfo,
                                    sensor_slave_addr, _ONSEMI_AR0144_CALIB_DATA_READ_ADDR, &QRegValue,
                                    FG_Address_2Byte | FG_Value_2Byte, nSensorMode);
    else
        ret = APC_GetSlaveSensorRegister(selfCalibration2Dlg->m_hApcDI, &selfCalibration2Dlg->m_DevSelInfo,
                                         sensor_slave_addr, _ONSEMI_AR0144_CALIB_DATA_READ_ADDR, &QRegValue,
                                         FG_Address_2Byte | FG_Value_2Byte, nSensorMode);
#elif __linux__
    ret = APC_GetSensorRegister(pHandleAPC, devSelInfo, sensor_slave_addr, _ONSEMI_AR0144_CALIB_DATA_READ_ADDR,
                                &QRegValue, FG_Address_2Byte | FG_Value_2Byte, (SENSORMODE_INFO) nSensorMode);
#endif

    if (ret != APC_OK) {
        std::cout << "APC_GetSensorRegister Failed." << std::endl;
        return -1;
    }

    // change for linux version by Brook at 20230913
    std::bitset<16>b = QRegValue;
    std::bitset<16>a;
    for (int i = 8; i < 16; i++)
    {
        a[i - 8] = b[i];
    }

    for (int i = 0; i < 8; i++)
    {
        a[i + 8] = b[i];
    }


    unsigned long Valuea = a.to_ullong();

    //unsigned long Valuea = 443.2;
    // change for linux version by Brook at 20230913
#ifdef _WIN32
    if (!selfCalibration2Dlg->m_isSensorSlave)
        ret = APC_GetSensorRegister(selfCalibration2Dlg->m_hApcDI, &selfCalibration2Dlg->m_DevSelInfo, sensor_slave_addr, _ONSEMI_AR0144_TEMP_READ_ADDR, &QRegValue, FG_Address_2Byte | FG_Value_2Byte, nSensorMode);
    else
        ret = APC_GetSlaveSensorRegister(selfCalibration2Dlg->m_hApcDI, &selfCalibration2Dlg->m_DevSelInfo, sensor_slave_addr, _ONSEMI_AR0144_TEMP_READ_ADDR, &QRegValue, FG_Address_2Byte | FG_Value_2Byte, nSensorMode);
#elif __linux__
    ret = APC_GetSensorRegister(pHandleAPC, devSelInfo, sensor_slave_addr, _ONSEMI_AR0144_TEMP_READ_ADDR, &QRegValue,
                                FG_Address_2Byte | FG_Value_2Byte, (SENSORMODE_INFO) nSensorMode);
#endif

    if (ret != APC_OK) {
        std::cout << "APC_GetSensorRegister Failed." << std::endl;
        fTemperature = NAN;
        return -1;
    }

    // change for linux version by Brook at 20230913
    b = QRegValue;

    for (int i = 8; i < 16; i++)
    {
        a[i - 8] = b[i];
    }

    for (int i = 0; i < 8; i++)
    {
        a[i + 8] = b[i];
    }

    unsigned long Valueb = a.to_ullong();
    //cout << "Valuea:" << Valuea << endl;
    //cout << "Valueb:" << Valueb << endl;

    fTemperature = (float)(_ONSEMI_AR0144_TEMP_CURVE_SLOPE * ((float)Valueb - (float)Valuea) + _ONSEMI_AR0144_CALIB_DATA_TEMP);

    return 0;
}

int CSelfCalibration2Controller::get_temperature_of_img_sensor_ar0135(void* pHandleAPC, E_THERMAL_SENSOR_MODEL e_sensor_model, int sensor_slave_addr, int nSensorMode, float& fTemperature) {

    int ret = 0;

    // change for linux version by Brook at 20230913
#ifdef _WIN32
    uint16_t RegValue;
    uint16_t QRegValue;
#elif __linux__
    unsigned short RegValue;
    unsigned short QRegValue;
#endif

#ifdef WIN32
    if (!selfCalibration2Dlg->m_isSensorSlave)
        ret = APC_SetSensorRegister(selfCalibration2Dlg->m_hApcDI, &selfCalibration2Dlg->m_DevSelInfo, sensor_slave_addr, _ONSEMI_AR0135_PRESET_ADDR, _ONSEMI_AR0135_PRESET_VALUE, FG_Address_2Byte | FG_Value_2Byte, nSensorMode);
    else
        ret = APC_SetSlaveSensorRegister(selfCalibration2Dlg->m_hApcDI, &selfCalibration2Dlg->m_DevSelInfo, sensor_slave_addr, _ONSEMI_AR0135_PRESET_ADDR, _ONSEMI_AR0135_PRESET_VALUE, FG_Address_2Byte | FG_Value_2Byte, nSensorMode);
#elif __linux__
    DEVSELINFO *devSelInfo = m_deviceSelInfoList[0];
    ret = APC_SetSensorRegister(pHandleAPC, devSelInfo, sensor_slave_addr,
                                _ONSEMI_AR0135_PRESET_ADDR, _ONSEMI_AR0135_PRESET_VALUE, FG_Address_2Byte|FG_Value_2Byte,
                                (SENSORMODE_INFO) nSensorMode);
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if (ret != APC_OK) {
        std::cout << "APC_SetSensorRegister Failed." << std::endl;
        return -1;
    }
#ifdef WIN32
    if (!selfCalibration2Dlg->m_isSensorSlave)
        ret = APC_GetSensorRegister(selfCalibration2Dlg->m_hApcDI, &selfCalibration2Dlg->m_DevSelInfo, sensor_slave_addr, _ONSEMI_AR0135_CALIB_DATA_READ_ADDR, &QRegValue, FG_Address_2Byte | FG_Value_2Byte, nSensorMode);
    else
        ret = APC_GetSlaveSensorRegister(selfCalibration2Dlg->m_hApcDI, &selfCalibration2Dlg->m_DevSelInfo, sensor_slave_addr, _ONSEMI_AR0135_CALIB_DATA_READ_ADDR, &QRegValue, FG_Address_2Byte | FG_Value_2Byte, nSensorMode);
#elif __linux__
    ret = APC_GetSensorRegister(pHandleAPC, devSelInfo, sensor_slave_addr,
                                _ONSEMI_AR0135_CALIB_DATA_READ_ADDR, &QRegValue, FG_Address_2Byte | FG_Value_2Byte,
                                (SENSORMODE_INFO) nSensorMode);
#endif
    if (ret != APC_OK) {
        std::cout << "APC_GetSensorRegister Failed." << std::endl;
        return -1;
    }

    // change for linux version by Brook at 20230913
    std::bitset<16>b = QRegValue;
    std::bitset<16>a;
    for (int i = 8; i < 16; i++)
    {
        a[i - 8] = b[i];
    }

    for (int i = 0; i < 8; i++)
    {
        a[i + 8] = b[i];
    }

    unsigned long Valuea = a.to_ullong();

    // change for linux version by Brook at 20230913
#ifdef WIN32
    if (!selfCalibration2Dlg->m_isSensorSlave)
        APC_GetSensorRegister(selfCalibration2Dlg->m_hApcDI, &selfCalibration2Dlg->m_DevSelInfo, sensor_slave_addr, _ONSEMI_AR0135_TEMP_READ_ADDR, &QRegValue, FG_Address_2Byte | FG_Value_2Byte, nSensorMode);
    else
        APC_GetSlaveSensorRegister(selfCalibration2Dlg->m_hApcDI, &selfCalibration2Dlg->m_DevSelInfo, sensor_slave_addr, _ONSEMI_AR0135_TEMP_READ_ADDR, &QRegValue, FG_Address_2Byte | FG_Value_2Byte, nSensorMode);
#elif __linux__
    ret = APC_GetSensorRegister(pHandleAPC, devSelInfo, sensor_slave_addr, _ONSEMI_AR0135_TEMP_READ_ADDR, &QRegValue,
                                FG_Address_2Byte | FG_Value_2Byte, (SENSORMODE_INFO) nSensorMode);
#endif
    if (ret != APC_OK) {
        std::cout << "APC_GetSensorRegister Failed." << std::endl;
        fTemperature = NAN;
        return -1;
    }

    // change for linux version by Brook at 20230913
    b = QRegValue;

    for (int i = 8; i < 16; i++)
    {
        a[i - 8] = b[i];
    }

    for (int i = 0; i < 8; i++)
    {
        a[i + 8] = b[i];
    }

    unsigned long Valueb = a.to_ullong();

    fTemperature = (float)(_ONSEMI_AR0135_TEMP_CURVE_SLOPE * ((float)Valueb - (float)Valuea) + _ONSEMI_AR0135_CALIB_DATA_TEMP);

    return 0;
}

int CSelfCalibration2Controller::get_temperature_of_img_sensor_st_vd5x(void* pHandleAPC, E_THERMAL_SENSOR_MODEL e_sensor_model, int sensor_slave_addr, int nSensorMode, float& fTemperature) {
    std::cout << ">> get_temperature_of_img_sensor_st_vd5x: " << std::endl;

    // ivy2: sensor_id = 0x20

    int ret = APC_OK;
    unsigned short TempeRegVal = 0x00;
    bool is_negtive = false;
    const unsigned short ADDR = _STM_VD5X_TEMP_READ_ADDR;
    // change for linux version by Brook at 20230913
    DEVSELINFO *devSelInfo = m_deviceSelInfoList[0];
    ret = APC_GetSensorRegister(pHandleAPC, devSelInfo, sensor_slave_addr, ADDR, &TempeRegVal,
                                FG_Address_2Byte | FG_Value_2Byte, (SENSORMODE_INFO) nSensorMode);

    // change for linux version by Brook at 20230913
    // HardCode Rule
    if (ret == APC_OK) {
        fTemperature = (float)TempeRegVal;
    } else {
        std::cout << "APC_GetSensorRegister() failed." << std::endl;
        fTemperature = NAN;
        return -1;
    }

    return 0;
}

int CSelfCalibration2Controller::get_temperature_of_ti_tmp_xxxx(void* pHandleAPC, E_THERMAL_SENSOR_MODEL e_sensor_model,
                                                                int sensor_slave_addr, int nSensorMode,
                                                                float& fTemperature) {

    std::cout << "@camera_device::get_temperature_of_ti_tmp1075()" << std::endl;
    bool b_successful = false;
    fTemperature = NAN;

    int ret = APC_OK;
    unsigned short TempeRegVal = 0x00;
    bool is_negtive = false;
    const unsigned short ADDR = 0x00;
    // change for linux version by Brook at 20230913
    DEVSELINFO* devSelInfo = m_deviceSelInfoList[0];
    ret = APC_GetSensorRegister(pHandleAPC, devSelInfo, sensor_slave_addr, ADDR, &TempeRegVal,
                                FG_Address_1Byte | FG_Value_2Byte, (SENSORMODE_INFO) SENSOR_BOTH);

    // change for linux version by Brook at 20230913
    // HardCode Rule
    if (ret == APC_OK) {
        unsigned short temperature_reg_val_reverse = ((((unsigned char*)&TempeRegVal)[0] << 8) +
                                                     ((unsigned char*)&TempeRegVal)[1]);
        TempeRegVal = temperature_reg_val_reverse;
        TempeRegVal >>= 5;
        fTemperature = (float)TempeRegVal * 0.125;
        b_successful = true;
    } else {
        std::cout << "get_temperature_of_ti_tmp1075() failed." << std::endl;
    }

    if (b_successful) {
        std::cout << "get temperature successfully. " << std::endl;
        return 0;
    } else {
        std::cout << "WARNING:  get temperature failed(). " << std::endl;
        return -1;
    }

}

int CSelfCalibration2Controller::get_temperature(void *pHandleAPC, E_THERMAL_SENSOR_MODEL e_sensor_model,
                                                 int sensor_slave_addr, int nSensorMode, float &fTemperature) {

    std::cout << "@camera_device::get_temperature()" << std::endl;
    bool b_successful = false;

    switch (e_sensor_model) {

        case E_THERMAL_SENSOR_MODEL::ONSEMI_AR0144:
        {
            int ret = get_temperature_of_img_sensor_ar0144(pHandleAPC, e_sensor_model, sensor_slave_addr,  nSensorMode, fTemperature);
            if (ret < 0)
                b_successful = false;
            else
                b_successful = true;
        }
            break;

        case E_THERMAL_SENSOR_MODEL::ONSEMI_AR0135:
        {
            int ret = get_temperature_of_img_sensor_ar0135(pHandleAPC, e_sensor_model, sensor_slave_addr,  nSensorMode, fTemperature);
            if (ret < 0)
                b_successful = false;
            else
                b_successful = true;
        }
            break;

        case E_THERMAL_SENSOR_MODEL::STM_VD5X:
        {
            int ret = get_temperature_of_img_sensor_st_vd5x(pHandleAPC, e_sensor_model, sensor_slave_addr,  nSensorMode, fTemperature);
            if (ret < 0)
                b_successful = false;
            else
                b_successful = true;
        }
            break;

        case E_THERMAL_SENSOR_MODEL::OV_OG02:
        case E_THERMAL_SENSOR_MODEL::OV_OG01:
        {
        }
            b_successful = true;
            break;
        case E_THERMAL_SENSOR_MODEL::TI_TMP1075: {
            int ret = get_temperature_of_ti_tmp_xxxx(pHandleAPC, e_sensor_model, sensor_slave_addr, nSensorMode, fTemperature);
            if (ret < 0)
                b_successful = false;
            else
                b_successful = true;
        }
            break;
        default:
            std::cout << "NO Matching Senor Model." << std::endl;
            break;
    }


    if (b_successful) {
        std::cout << "get temperature successfully. " << std::endl;
        return 0;
    }
    else {
        std::cout << "WARNING:  get temperature failed(). " << std::endl;
        return -1;
    }

}

static const CSelfCalibration2Controller::PARAM_t DEFAULT_REPAIR_PARAMS = {
    true, 5.0, static_cast<int>(SelfK2::C_Cy_Compensator::E_ESTIMATOR_TYPES::_confidence_weighted_ranking), 0.00, 0.98, 1, 30, 0.0
};

static const CSelfCalibration2Controller::PARAM_t DEFAULT_RUNTIME_PARAMS = {
    1, 1, true, 1.0, 60.0, 0.15, 1, 0.10,
    true, 2.5, SelfK2::C_Cy_Compensator::E_ESTIMATOR_TYPES::_confidence_weighted_ranking, 0.00, 0.997, 3, 30, 0.5
};

CSelfCalibration2Controller::CSelfCalibration2Controller(void *APCHandle, DEVSELINFO *pDevSelInfo,
                                                         size_t width, size_t height):
    m_currentMode(Mode::DEPTH_BROKEN_REPAIR) {
    mAPCHandle = APCHandle;
    m_deviceSelInfoList.push_back(pDevSelInfo);
    mWidth = width;
    mHeight = height;

    APC_GetDeviceInfoEx(mAPCHandle, m_deviceSelInfoList[0], &mDevInformationEx);
    mProductId = mDevInformationEx.wPID;
    m_RectifyData = std::unique_ptr<eSPCtrl_RectLogData>(new eSPCtrl_RectLogData());
}

CSelfCalibration2Controller::~CSelfCalibration2Controller() {
    stopThreads();
    m_RectifyData.reset();
    m_focal_compensator.reset();
    m_cy_compensator.reset();
}

bool CSelfCalibration2Controller::IsDepthStreaming() {
    return true;
}

void CSelfCalibration2Controller::SwapDepthImage(std::vector<uint8_t> depthBuffer) {
    std::lock_guard<std::mutex> lock(mDepthMutex);
    if (m_Depth.size() != depthBuffer.size()) {
        m_Depth.resize(depthBuffer.size());
    }
    m_Depth.swap(depthBuffer);
}

int CSelfCalibration2Controller::RunSelfK2() {
    if (m_isRunning.load()) {
        return -1;
    }

    if (!IsDepthStreaming()) {
        return -1;
    }

    stopThreads();

    float calib_temperature = m_RectifyData->LR_cam_K_temperature[_1st_];
    float LR_cam_thermal_variation_rate_of_focal = m_RectifyData->LR_cam_thermal_variation_rate_of_focal[_1st_];
    if (GetMode() == Mode::RUNTIME_CORRECTION) {
        memcpy(&m_cy_compensator_param, &param_runtime, sizeof(struct PARAM_t));
    } else {
        memcpy(&m_cy_compensator_param, &param_repair, sizeof(struct PARAM_t));
    }

    void* pHandleAPC = mAPCHandle;
    if (!pHandleAPC) return false;
    SelfK2::set_hw_handler(pHandleAPC, m_deviceSelInfoList[0], &mDevInformationEx);

    auto depthWidth = mWidth;
    auto depthHeight= mHeight;

    m_cy_compensator.reset(new SelfK2::C_Cy_Compensator(
            (int) m_cy_compensator_param.mean_shift_kernel_size_acquisition,
            (int) m_cy_compensator_param.mean_shift_kernel_size_tracking
    ));
    m_cy_compensator->pars.n_cols = depthWidth;
    m_cy_compensator->pars.n_rows = depthHeight;
    m_cy_compensator->pars.b_auto_adjust_period = m_cy_compensator_param.b_auto_adjust_period;
    m_cy_compensator->pars.update_period_in_seconds_of_cy_in_acquisition = m_cy_compensator_param.update_period_in_seconds_of_cy_in_acquisition;
    m_cy_compensator->pars.update_period_in_seconds_of_cy_in_tracking = m_cy_compensator_param.update_period_in_seconds_of_cy_in_tracking;
    m_cy_compensator->pars.cy_sampling_period_in_seconds = m_cy_compensator_param.cy_sampling_period_in_seconds;
    m_cy_compensator->pars.temperatureThreshold = m_cy_compensator_param.temperatureThreshold;
    m_cy_compensator->pars.valid_min_fill_rate_threshold = m_cy_compensator_param.valid_min_fill_rate_threshold;
    m_cy_compensator->pars.b_dynamic_kernel_size = m_cy_compensator_param.b_dynamic_kernel_size;
    m_cy_compensator->pars.max_devication_of_cy = m_cy_compensator_param.max_devication_of_cy;
    m_cy_compensator->pars.e_estimator_type = (SelfK2::C_Cy_Compensator::E_ESTIMATOR_TYPES)m_cy_compensator_param.e_estimator_type;
    m_cy_compensator->pars.blind_zone_ratio = m_cy_compensator_param.blind_zone_ratio;
    m_cy_compensator->pars.fillrate_threshold_to_enter_tracking = m_cy_compensator_param.fillrate_threshold_to_enter_tracking;
    m_cy_compensator->pars.convergency_cnt_threshold = m_cy_compensator_param.convergency_cnt_threshold;
    m_cy_compensator->pars.max_acq_iterations = m_cy_compensator_param.max_acq_iterations;
    m_cy_compensator->pars.smoothing_factor = m_cy_compensator_param.smoothing_factor;

    if (LR_cam_thermal_variation_rate_of_focal != 0) {
        m_focal_compensator.reset(new SelfK2::C_Focal_Compensator(calib_temperature, LR_cam_thermal_variation_rate_of_focal));
    }

    m_isRunning.store(true);

    m_temperatureThread.reset(new std::thread(&CSelfCalibration2Controller::temperatureMonitorThread, this));
    m_compensatorThread.reset(new std::thread(&CSelfCalibration2Controller::compensatorThread, this));

    return 0;
}

void CSelfCalibration2Controller::StopSelfK2() {
    stopThreads();
}

void CSelfCalibration2Controller::ResetSelfK2() {
    Reset();
}

bool CSelfCalibration2Controller::WriteToFlash(float compensateCy, int index) {
    std::vector<uint8_t> readRectifyBuffer(APC_RECTIFY_FILE_SIZE);

    int actualLen = 0;
    void* pHandleAPC = mAPCHandle;
    if (!pHandleAPC) return false;

    auto calibrationFileIndex = index;
    int ret = APC_GetRectifyTable(pHandleAPC, m_deviceSelInfoList[0], readRectifyBuffer.data(), APC_RECTIFY_FILE_SIZE,
                                  &actualLen, calibrationFileIndex);

    if (ret != APC_OK) return false;

    BinaryParser parser(readRectifyBuffer);
    // parser.printData();

    // Keep the same magnification_for_setting_register_cy it's a private in SelfK2::C_Cy_Compensator::Pars.
    uint16_t registerValue = compensateCy * 4.0f;
    uint8_t  registerLowByte = registerValue & 0xFF;
    uint8_t  registerHighByte = registerValue >> 8u;

    bool patchSuccess = parser.patchValue(PUMA_REG_ADDR_CY2[0], registerLowByte);
    patchSuccess &= parser.patchValue(PUMA_REG_ADDR_CY2[1], registerHighByte);
    if (!patchSuccess) {
        return patchSuccess;
    }

    ret = APC_SetRectifyTable(pHandleAPC, m_deviceSelInfoList[0], parser.exportData().data(), APC_RECTIFY_FILE_SIZE,
                              &actualLen, calibrationFileIndex);

    return ret == APC_OK;
}

void CSelfCalibration2Controller::SetMode(Mode mode) {
    Mode currentMode = m_currentMode.load();
    if (currentMode != mode) {
        m_currentMode.store(mode);
    }
}

void CSelfCalibration2Controller::SetCurrentSensorTemperature(float temperature) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentTemperature = temperature;
}

float CSelfCalibration2Controller::GetCurrentSensorTemperature() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentTemperature;
}

void CSelfCalibration2Controller::temperatureMonitorThread() {
    std::stringstream ss;
    ss << std::this_thread::get_id();

    void* pHandleAPC = mAPCHandle;
    if (!pHandleAPC) {
        fprintf(stderr, "No handler check it\n");
    }

    E_THERMAL_SENSOR_MODEL sensorModel = E_THERMAL_SENSOR_MODEL::ONSEMI_AR0135;
    int sensorAddress = 0;
    int sensorMode = SENSOR_A;

    uint16_t pid = mProductId;
    get_temperature_param(pid, sensorModel, sensorAddress, sensorMode);

    while (m_isRunning.load()) {
        float temperature;
        if (!IsDepthStreaming()) {
            m_isRunning.store(false);
            break;
        }

        int ret = get_temperature(pHandleAPC, sensorModel, sensorAddress, sensorMode, temperature);
        if (ret < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        SetCurrentSensorTemperature(temperature);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

static void *g_pHandleApcDI = nullptr;
static DEVSELINFO* g_pDevSelInfo = nullptr;
static DEVINFORMATIONEX* g_pDevinfo = nullptr;

void SelfK2::set_hw_handler(
        void* pHandleApcDI,
        DEVSELINFO* pDevSelInfo,
        DEVINFORMATIONEX* pDevinfo // @20230825
) {
    g_pHandleApcDI = pHandleApcDI;
    g_pDevSelInfo = pDevSelInfo;
    g_pDevinfo = pDevinfo;
}

int SelfK2::C_Focal_Compensator::set_focal_reg_addrs(unsigned short* p_reg_addrs_fx1,
                                                     unsigned short* p_reg_addrs_fy1,
                                                     unsigned short* p_reg_addrs_fx2,
                                                     unsigned short* p_reg_addrs_fy2) {

    memcpy(pars.reg_addrs_fx1, p_reg_addrs_fx1, sizeof(unsigned short) * 3);
    memcpy(pars.reg_addrs_fy1, p_reg_addrs_fy1, sizeof(unsigned short) * 3);
    memcpy(pars.reg_addrs_fx2, p_reg_addrs_fx2, sizeof(unsigned short) * 3);
    memcpy(pars.reg_addrs_fy2, p_reg_addrs_fy2, sizeof(unsigned short) * 3);

    return (int)E_ERROR::_OK;
}

int SelfK2::C_Focal_Compensator::_read_reg_and_set_to_the_init_focal() {
    init_fx1 = Get_HW_Reg_3Bytes_Data(pars.reg_addrs_fx1, pars.magnification_for_setting_register_focal);
    init_fy1 = Get_HW_Reg_3Bytes_Data(pars.reg_addrs_fy1, pars.magnification_for_setting_register_focal);
    init_fx2 = Get_HW_Reg_3Bytes_Data(pars.reg_addrs_fx2, pars.magnification_for_setting_register_focal);
    init_fy2 = Get_HW_Reg_3Bytes_Data(pars.reg_addrs_fy2, pars.magnification_for_setting_register_focal);
    return (int)E_ERROR::_OK;
}

int SelfK2::C_Focal_Compensator::_reset_focal_reg_to_the_init_value() {
    Set_HW_Reg_3Bytes_Data(pars.reg_addrs_fx1, init_fx1, 8.0);
    Set_HW_Reg_3Bytes_Data(pars.reg_addrs_fx1, init_fx1, 8.0);
    Set_HW_Reg_3Bytes_Data(pars.reg_addrs_fx1, init_fx1, 8.0);
    Set_HW_Reg_3Bytes_Data(pars.reg_addrs_fx1, init_fx1, 8.0);
    return (int)E_ERROR::_OK;
}

int SelfK2::C_Focal_Compensator::_reset() {
    _reset_focal_reg_to_the_init_value();
    period_for_running_focal_section = pars.update_period_in_seconds_of_focal_init;

    T_now = clock();
    T_last_focal = T_now;
    last_temperature = current_temperature;

    return (int)E_ERROR::_OK;
}

int SelfK2::C_Focal_Compensator::run(float current_temp) {
    if (pars.alpha == 0) {
        return (int)E_ERROR::_Not_Valid_LR_cam_thermal_variation_rate_of_focal;
    }

    if (std::isnan(pars.calib_temperature) == true) {
        return (int)E_ERROR::_Not_Valid_Calibration_Temperature;
    }

    if (std::isnan(current_temp) == true) {
        return (int)E_ERROR::_Not_Valid_Current_Temperature;
    }

    T_now = clock();

    double time_diff = (T_now - T_last_focal) / (double)CLOCKS_PER_SEC; // sec

    current_temperature = current_temp;
    float temp_diff = (current_temperature - last_temperature);


    if ((time_diff > period_for_running_focal_section) || (abs(temp_diff) > pars.temperatureThreshold)) {

        info.e_states = E_STATES::_estimating;

        std::cout << "temp_diff@focal_estimation: " << temp_diff << std::endl;
        std::cout << "time_diff@focal_estimation : " << time_diff << std::endl;

        last_temperature = current_temperature;
        T_last_focal = T_now;
        period_for_running_focal_section = pars.update_period_in_seconds_of_focal_ordinary;

        float temp_diff = current_temp - pars.calib_temperature;
        float gain = 1.0 + pars.alpha * temp_diff;
        float fx1 = 0, fy1 = 0;
        float fx2 = 0, fy2 = 0;

        //
        std::cout << "focal scaling : " << gain << std::endl;

        fx1 = init_fx1 * (1.0 + pars.alpha * temp_diff);
        fy1 = init_fy1 * (1.0 + pars.alpha * temp_diff);
        fx2 = init_fx2 * (1.0 + pars.alpha * temp_diff);
        fy2 = init_fy2 * (1.0 + pars.alpha * temp_diff);

        Set_HW_Reg_3Bytes_Data(pars.reg_addrs_fx1, fx1, pars.magnification_for_setting_register_focal);
        Set_HW_Reg_3Bytes_Data(pars.reg_addrs_fy1, fy1, pars.magnification_for_setting_register_focal);
        Set_HW_Reg_3Bytes_Data(pars.reg_addrs_fx2, fx2, pars.magnification_for_setting_register_focal);
        Set_HW_Reg_3Bytes_Data(pars.reg_addrs_fy2, fy2, pars.magnification_for_setting_register_focal);

        oputs.comp_fx1 = fx1;
        oputs.comp_fy1 = fy1;
        oputs.comp_fx2 = fx2;
        oputs.comp_fy2 = fy2;

    }
    else {
        info.e_states = E_STATES::_idle;
    }

    return (int)E_ERROR::_OK;
}

int SelfK2::C_Cy_Compensator::_reset_pars_and_status(bool f_in_acquisition, float center_cy) {
    std::cout << ">> _reset_pars_and_status()." << std::endl;


    b_bypass = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    n_persuit = 0;
    f_error = false;

    info.e_states = E_STATES::_idle;

    b_locked_in_acquisition_mode = false;
    this->f_in_acquisition = f_in_acquisition;
    f_stable_mode_switch = false;
    current_temperature = 0;
    last_temperature = 0;
    period_for_running_oc_section = pars.update_period_in_seconds_of_cy_in_acquisition;

    i_update_pars = 0;
    accmulated_cnt = 0;
    i_order = 0;
    i_test = 0;
    i_successfully_test = 0;

    if (std::isnan(center_cy) == false) {
        this->center_cy = center_cy;
    }
    else {
        this->center_cy = default_cy;
    }


    last_center_cy = 0;
    estimated_cy = this->center_cy;
    cy_to_update = 0;

    if (this->f_in_acquisition)
        mean_shift_kernel_size = pars.mean_shift_kernel_size_for_acquisition;
    else
        mean_shift_kernel_size = pars.mean_shift_kernel_size_for_tracking;

    _update_ms_kernel_size(mean_shift_kernel_size);


    int  n_samples = mean_shift_kernel_size * 2 + 1;
    for (int i = 0; i < n_samples; i++) {
        p_test_cys[i] = 0;
        p_fill_rates[i] = 0;
        p_rank_of_fill_rates[i] = 0;
    }
    n_center_cy_no_change = 0; // reset


    if (p_c_var_estimator_fill_rate != nullptr) {
        delete p_c_var_estimator_fill_rate;
        p_c_var_estimator_fill_rate = new RollingVariance(pars.window_size_for_variance_estimation_fill_rate);
    }
    else {
        std::cout << "WARNING: p_c_var_estimator_fill_rate is nullptr." << std::endl;
    }

    if (p_c_var_estimator_cy != nullptr) {
        delete p_c_var_estimator_cy;
        p_c_var_estimator_cy = new RollingVariance(pars.window_size_for_variance_estimation_cy);
    }
    else {
        std::cout << "WARNING: p_c_var_estimator_cy is nullptr." << std::endl;
    }

    if (p_c_var_estimator_temp != nullptr) {
        delete p_c_var_estimator_temp;
        p_c_var_estimator_temp = new RollingVariance(pars.window_size_for_variance_estimation_temp);
    }
    else {
        std::cout << "WARNING: p_c_var_estimator_temp is nullptr." << std::endl;
    }

    // release
    b_bypass = false;

    return (int)E_ERROR::_OK;
}

int SelfK2::C_Cy_Compensator::acq_or_tracking_mode_switch() {
    std::cout<<"acq_or_tracking_mode_switch : " << std::endl;

    bool b_tmp = f_in_acquisition;
    _reset_pars_and_status(!b_tmp, center_cy);

    std::cout<<"f_in_acquisition: "<< f_in_acquisition << std::endl;
    std::cout << "f_stable_mode_switch: " << f_stable_mode_switch << std::endl;
    return 0;
}

int SelfK2::C_Cy_Compensator::_set_current_cy(float cy) {
    std::cout << ">> _set_current_cy()." << std::endl;

    _reset();
    Set_HW_Reg_2Bytes_Data(pars.reg_addrs_cy, cy, pars.magnification_for_setting_register_cy);
    center_cy = round(cy* pars.magnification_for_setting_register_cy) / pars.magnification_for_setting_register_cy;

    return (int)E_ERROR::_OK;
}

float SelfK2::C_Cy_Compensator::_get_current_cy() {
    return center_cy;
}

int SelfK2::C_Cy_Compensator::_reset(float init_cy) {
    std::cout << "_reset()" << std::endl;

    _reset_pars_and_status();

    T_now = clock();
    T_last_oc = T_now;

    // reset cy to default value
    if (std::isnan(init_cy) == false)
        Set_HW_Reg_2Bytes_Data(pars.reg_addrs_cy, init_cy, pars.magnification_for_setting_register_cy);
    else
        Set_HW_Reg_2Bytes_Data(pars.reg_addrs_cy, default_cy, pars.magnification_for_setting_register_cy);

    return (int)E_ERROR::_OK;
}

int SelfK2::C_Cy_Compensator::_update_ms_kernel_size(int _size) {
    mean_shift_kernel_size = _size;
    n_mean_shift_samples = 2 * mean_shift_kernel_size + 1;

    if (p_test_cys != nullptr)
        delete[] p_test_cys;

    if (p_fill_rates != nullptr)
        delete[] p_fill_rates;

    if (p_rank_of_fill_rates != nullptr)
        delete[] p_rank_of_fill_rates;

    if (p_increments != nullptr)
        delete[] p_increments;

    p_test_cys = new float[n_mean_shift_samples];//(float*)malloc(mean_shift_kernel_size * sizeof(float));
    p_fill_rates = new float[n_mean_shift_samples];//(float*)malloc(mean_shift_kernel_size * sizeof(float));
    p_rank_of_fill_rates = new int[n_mean_shift_samples];//(float*)malloc(mean_shift_kernel_size * sizeof(float));
    p_increments = new float[n_mean_shift_samples];//(float*)malloc(mean_shift_kernel_size * sizeof(float));

    // info:
    info.v_fill_rate_of_cys.clear();
    info.v_fill_rate_of_cys.resize(n_mean_shift_samples);
    info.v_test_cys.clear();
    info.v_test_cys.resize(n_mean_shift_samples);

    // Initialize the array to zero using a loop
    for (int i = 0; i < n_mean_shift_samples; i++) {
        p_test_cys[i] = 0;
        p_fill_rates[i] = 0;
        p_rank_of_fill_rates[i] = 0;
        p_increments[i] = 0;
    }
    n_center_cy_no_change = 0; // reset

    for (int i = 0; i < mean_shift_kernel_size; i++) {
        p_increments[i * 2 + 0] = -(i + 1);
        p_increments[i * 2 + 1] = (i + 1);
    }

    return (int)E_ERROR::_OK;
}

template<typename T>
double calculateNonZeroRatio(const T* data, int size, int n_cols, int n_rows, float blind_zone_ratio, bool b_subwin,
                             int sx, int sy, int roi_w, int roi_h) {
    if (data == nullptr || size <= 0) {
        return -1.0;
    }

    const int tmp = n_cols * blind_zone_ratio;
    int cnt = 0;
    int nonZeroCount = 0;

    if (b_subwin) {
        for (int iy = sy; iy < sy + roi_h; iy++) {
            for (int ix = sx; ix < sx + roi_w; ix++) {
                if (ix > tmp) {
                    cnt++;
                    int idx = iy * n_cols + ix;
                    if (data[idx] != 0)
                        nonZeroCount++;
                }
            }
        }
    }
    else {
        for (int i = 0; i < size; ++i) {
            if ((i % n_cols) > tmp) {
                cnt++;
                if (data[i] != 0)
                    nonZeroCount++;
            }
        }

    }


    if (cnt == 0) {
        return 0.0;
    }

    return static_cast<double>(nonZeroCount) / (float)cnt;
}

template<typename T>
double calculateNonZeroRatio(const T* data, int size) {
    if (data == nullptr || size <= 0) {
        return -1.0;
    }

    int nonZeroCount = 0;
    for (int i = 0; i < size; ++i) {
        if (data[i] != 0) {
            nonZeroCount++;
        }
    }

    if (size == 0) {
        return 0.0;
    }

    return static_cast<double>(nonZeroCount) / size;
}
void getSortedOrder(const float arr[], int sortedOrder[], int size, int& minIndex, int& maxIndex) {
    // Create a copy to preserve the original array
    std::vector<float> sortedArr(arr, arr + size);

    // Use the STL sort function to sort the copy
    std::sort(sortedArr.begin(), sortedArr.end());

    // Find the position of each element in the sorted array and store it in sortedOrder
    for (int i = 0; i < size; ++i) {
        auto it = std::lower_bound(sortedArr.begin(), sortedArr.end(), arr[i]);
        if (it != sortedArr.end()) {
            sortedOrder[i] = std::distance(sortedArr.begin(), it);
        }
    }

    // Find the index of the minimum and maximum values in the original array
    minIndex = std::distance(arr, std::min_element(arr, arr + size));
    maxIndex = std::distance(arr, std::max_element(arr, arr + size));
}

// Define a function to find the maximum value in an array
float _findMax(const float arr[], int size) {
    if (size <= 0) {
        std::cerr << "Invalid array size" << std::endl;
        return 0.0;
    }

    float maxVal = arr[0];  // Assume the first element is the maximum value

    for (int i = 1; i < size; i++) {
        if (arr[i] > maxVal) {
            maxVal = arr[i];  // Update the maximum value
        }
    }

    return maxVal;
}
// Define a function to find the minimum value in an array
float _findMin(const float arr[], int size) {
    if (size <= 0) {
        std::cerr << "Invalid array size" << std::endl;
        return 0.0;
    }

    float minVal = arr[0];  // Assume the first element is the minimum value

    for (int i = 1; i < size; i++) {
        if (arr[i] < minVal) {
            minVal = arr[i];  // Update the minimum value
        }
    }

    return minVal;
}

template<typename T>
int SelfK2::C_Cy_Compensator::run(const T* p_depth, int n_depth_pixels, float current_temp) {
    if (b_bypass) {
        return 0;
    }

    T_now = clock();
    double time_diff = (T_now - T_last_oc) / (double)CLOCKS_PER_SEC; // sec

    current_temperature = current_temp;
    float temp_diff = (current_temperature - last_temperature);


    if ((time_diff > period_for_running_oc_section) || (abs(temp_diff) > pars.temperatureThreshold)) {



        std::cout << std::endl;
        std::cout << std::endl;
        std::cout << "temp_diff@oc_estimation: " << temp_diff << std::endl;
        std::cout << "time_diff@oc_estimation : " << time_diff << std::endl;

        // reset counter
        T_last_oc = T_now;
        last_temperature = current_temperature;

        std::cout << "do_temp_compensation: i_order" << i_order << std::endl;
        {




            // @init state: check if the depth image is ready for estimate cy variation (statistic info).

            if (f_estimating_cy == false) { // init
                float fill_rate = 0;
                if ((pars.n_cols > 0) && (pars.blind_zone_ratio > 0))
                    fill_rate = calculateNonZeroRatio(p_depth, n_depth_pixels, pars.n_cols, pars.n_rows, pars.blind_zone_ratio);
                else
                    fill_rate = calculateNonZeroRatio(p_depth, n_depth_pixels);

                if (fill_rate > pars.valid_min_fill_rate_threshold) {
                    f_estimating_cy = true;

                    if (pars.b_random_sampling) {
                        // init ROI:
                        sampling_roi = SelfK2::getRandomROI(pars.n_cols, pars.n_rows, min_random_sampling_roi_ratio, min_random_sampling_roi_ratio);
                        bool b_valid_roi = (sampling_roi.height > pars.n_rows * min_random_sampling_roi_ratio) && (sampling_roi.width > pars.n_cols * min_random_sampling_roi_ratio);

                        if (b_valid_roi == false)
                            sampling_roi = ROI(0, 0, pars.n_cols, pars.n_rows);

                        std::cout << "random sampling_roi: " << sampling_roi.sx << ", " << sampling_roi.sy << ", " << sampling_roi.width << ", " << sampling_roi.height << std::endl;
                    }
                }

                info.e_states = E_STATES::_init;
            }


            // depth image is ready ?
            if (f_estimating_cy) {
                info.e_states = E_STATES::_estimating;

                if (i_order == n_mean_shift_samples) { // sampling is done, next is estimating cy ...
                    i_order = 0;
                    i_test++;

                    if (pars.b_random_sampling) {
                        // init ROI:
                        sampling_roi = SelfK2::getRandomROI(pars.n_cols, pars.n_rows, min_random_sampling_roi_ratio, min_random_sampling_roi_ratio);
                        bool b_valid_roi = (sampling_roi.height > pars.n_rows * min_random_sampling_roi_ratio) && (sampling_roi.width > pars.n_cols * min_random_sampling_roi_ratio);
                        if (b_valid_roi == false)
                            sampling_roi = ROI(0, 0, pars.n_cols, pars.n_rows);

                        std::cout<<"random sampling_roi: "<< sampling_roi.sx << ", " << sampling_roi.sy << ", " << sampling_roi.width << ", " << sampling_roi.height << std::endl;
                    }

                    // check if the depth image has enough information --> confidence.
                    bool b_valid = false;
                    {

                        int n = 0;
                        for (int i = 0; i < n_mean_shift_samples; i++) { // check each cy-test resut

                            // average of fillrate (accmunated fillrate for mean-shift)
                            if ((pars.e_estimator_type == E_ESTIMATOR_TYPES::_mean_shift) && (n_center_cy_no_change > 1)) {
                                p_fill_rates[i] /= (float)n_center_cy_no_change;
                            }


                            std::cout << i << ", cy: " << p_test_cys[i] <<", FillRate:"<< p_fill_rates[i] << std::endl;
                            info.v_fill_rate_of_cys[i] = p_fill_rates[i];
                            info.v_test_cys[i] = p_test_cys[i];
                            if (p_fill_rates[i] > pars.valid_min_fill_rate_threshold) // check fill-rate?
                                n++;
                        }


                        if (n >= pars.n_valid_test_cys_threahold) // when at least n_valid_test_cys_threahold cy-test has enough fill-rate info --> ths test set is valid.
                            b_valid = true;
                    }


                    if (b_valid) { // sampling data is valid.
                        std::cout << "@i_order == n_mean_shift_samples :" << std::endl;
                        i_successfully_test++;

                        // when cy is NOT changed.
                        if (center_cy == last_center_cy) {
                            accmulated_cnt++;
                            std::cout << "accmulated_cnt:" << accmulated_cnt << std::endl;
                        }
                        else {
                            accmulated_cnt = 0; // reset
                        }


                        // estimate data variance --> noise of data --> update rate
                        double currentVariance = p_c_var_estimator_fill_rate->getVariance();
                        double current_stdev = 1000.0;
                        if (!std::isnan(currentVariance)) {

                            current_stdev = sqrt(currentVariance);
                            std::cout << "currentVariance: " << currentVariance << std::endl;
                            std::cout << "current_stdev: " << current_stdev << std::endl;
                        }
                        else {
                            std::cout << "No enough data for variance estimation." << std::endl;
                        }

                        // get the ranking of the cy-tests --> to qualify the winner = maxIndex_tmp.
                        int minIndex_tmp = 0;
                        int maxIndex_tmp = 0; //  0 = position of the center_cy
                        getSortedOrder(p_fill_rates, p_rank_of_fill_rates, n_mean_shift_samples, minIndex_tmp, maxIndex_tmp);
                        std::cout<<"Max Ranking index of Fill-Rate: "<< maxIndex_tmp << std::endl;


                        // to estimate cy
                        float estimated_cy_tmp = 0;
                        if (pars.e_estimator_type == E_ESTIMATOR_TYPES::_mean_shift) {
                            // mean shift
                            float wo = 2.0;
                            float min_fill_rate = _findMin(p_fill_rates, n_mean_shift_samples);
                            float max_fill_rate = _findMax(p_fill_rates, n_mean_shift_samples);
                            float* p_weightings = new float[n_mean_shift_samples];
                            float total_weight = 0;
                            float w_cy_sum = 0;
                            for (int i = 0; i < n_mean_shift_samples; i++) {
                                p_weightings[i] = p_fill_rates[i] - min_fill_rate;
                                p_weightings[i] = pow(p_weightings[i], wo);
                                total_weight += p_weightings[i];
                                w_cy_sum += p_weightings[i] * p_test_cys[i];
                            }
                            if (total_weight == 0) {
                                std::cout << "ERROR: total_weight is zero." << std::endl;
                                return -1;
                            }

                            estimated_cy_tmp = w_cy_sum / total_weight;

                            // smoothing
                            float alpha_s = 1.0 / (1.0 + current_stdev * 30 / (accmulated_cnt + 1.0));
                            std::cout << "alpha_s:" << alpha_s << std::endl;
                            estimated_cy = estimated_cy * (1.0 - alpha_s) + estimated_cy_tmp * alpha_s;
                        }
                        else if (pars.e_estimator_type == E_ESTIMATOR_TYPES::_confidence_weighted_ranking) {
                            estimated_cy_tmp = p_test_cys[maxIndex_tmp];
                            if (abs(estimated_cy_tmp - default_cy) < pars.max_devication_of_cy) {
                                if (maxIndex_tmp == last_max_fillrate_index) {
                                    n_winning_streak++;
                                }
                                else {
                                    n_winning_streak = 0;
                                }
                                last_max_fillrate_index = maxIndex_tmp;

                                // smoothing
                                float alpha_s = 1.0 - pow(0.5, n_winning_streak) * 0.95;
                                // float alpha_s = sigma / sqrt(n);
                                std::cout << "alpha_s:" << alpha_s << std::endl;
                                estimated_cy = estimated_cy * (1.0 - alpha_s) + estimated_cy_tmp * alpha_s;
                            }
                            // _confidence_weighted_ranking
                        }


                        oputs.estimaed_cy = estimated_cy;
                        float estimated_cy_dev = (estimated_cy - default_cy);
                        std::cout << "i_update_pars:" << i_update_pars << ", accmulated_cnt:" << accmulated_cnt << ", default_cy: " << default_cy << ",	center_cy : " << center_cy << ", estimated_cy_tmp: " << estimated_cy_tmp << ",	 estimated_cy:" << estimated_cy << ", cy_diff:" << estimated_cy_dev << ", " << std::endl;

                        // update data
                        if (abs(estimated_cy_dev) < pars.max_devication_of_cy) {
                            std::cout << ">> UPDATE cy value:" << round(estimated_cy * pars.magnification_for_setting_register_cy) / pars.magnification_for_setting_register_cy << std::endl;
                            Set_HW_Reg_2Bytes_Data(pars.reg_addrs_cy, estimated_cy, pars.magnification_for_setting_register_cy);
                            last_center_cy = center_cy;
                            i_update_pars++;
                            std::cout << "i_update_pars: " << i_update_pars << std::endl;
                        }
                        else {

                            std::cout << ">> WARNING: invalid cy value >> estimated_cy_dev > pars.max_devication_of_cy. >> cy NOT updated." << std::endl;
                            std::cout << "estimated_cy_dev:" << estimated_cy_dev << std::endl;
                        }


                        // status = in acquistion or tracking ?
                        if (f_in_acquisition == true) {
                            std::cout << ">>@Acquisition : (f_in_acquisition == true)" << std::endl;
                            float fillrate = p_fill_rates[0]; // sorted, so , index = 0 has max fillrate.
                            bool f_fillrate_reach_target = fillrate > pars.fillrate_threshold_to_enter_tracking;
                            bool f_center_cy_has_best_fillrate = (maxIndex_tmp == INDEX_OF_CENTER_CY);

                            // 2. maxIndex_tmp not equal to the index of the cenetr_cy, means , estimator is still in acquisition (not stable yet).
                            bool f_convergency_cnt_reach_target = ((maxIndex_tmp == INDEX_OF_CENTER_CY) && (accmulated_cnt > pars.convergency_cnt_threshold));
                            if (
                                    (f_fillrate_reach_target && f_center_cy_has_best_fillrate) ||
                                    f_convergency_cnt_reach_target
                                    ) {
                                if (b_locked_in_acquisition_mode == false) {
                                    f_in_acquisition = false;
                                    f_stable_mode_switch = true;
                                    //pars.e_estimator_type = E_ESTIMATOR_TYPES::_mean_shift;
                                    std::cout << ">> b_in_acquisition = false" << std::endl;
                                }
                            }
                        }
                        else { // tracking
                            std::cout << ">>@Tracking : (f_in_acquisition == false)" << std::endl;

                            //if(maxIndex_tmp != INDEX_OF_CENTER_CY) {
                            if (abs(estimated_cy -  center_cy) > pars.tracking_stable_range){

                                n_persuit++;
                                if (n_persuit == pars.tracking_fail_cnt_threshold) {
                                    n_persuit = 0;
                                    f_in_acquisition = true;
                                    //pars.e_estimator_type = E_ESTIMATOR_TYPES::_confidence_weighted_ranking;
                                    f_stable_mode_switch = true;
                                    std::cout << ">> b_in_acquisition = true" << std::endl;
                                }
                            }
                            else
                            {
                                n_persuit = 0;
                            }
                            std::cout << ">> n_persuit: " << n_persuit << std::endl;
                        }

                        // period adjustment
                        if (pars.b_auto_adjust_period) {
                            if (f_in_acquisition == true) { // NOT stable
                                period_for_running_oc_section = pars.update_period_in_seconds_of_cy_in_acquisition;
                            }
                            else { //  stable


                                if (std::isnan(current_temp)) { // no_temp >>  if no temp info, we smoothly increase the check period.
                                    period_for_running_oc_section = std::min((int)accmulated_cnt, (int)(pars.update_period_in_seconds_of_cy_in_tracking + 1));
                                    period_for_running_oc_section = std::max((int)pars.update_period_in_seconds_of_cy_in_acquisition, (int)period_for_running_oc_section);
                                }
                                else { // update oc when temp has enough increment.
                                    period_for_running_oc_section = pars.update_period_in_seconds_of_cy_in_tracking;
                                }
                            }
                        }
                        else {
                            period_for_running_oc_section = pars.update_period_in_seconds_of_cy_in_tracking;
                        }
                        std::cout << "period_for_running_oc_section: " << period_for_running_oc_section << std::endl;




                        // stable status changed!
                        if (f_stable_mode_switch) {
                            f_stable_mode_switch = false; // clear
                            if (f_in_acquisition == false) { // acquisition to tracking

                                if (pars.b_dynamic_kernel_size)
                                    _update_ms_kernel_size(pars.mean_shift_kernel_size_for_tracking);
                            }
                            else {
                                if (pars.b_dynamic_kernel_size) // tracking to acquisition
                                    _update_ms_kernel_size(pars.mean_shift_kernel_size_for_acquisition);
                            }
                        } // if (f_stable_mode_switch) {

                        p_c_var_estimator_cy->addDataPoint(estimated_cy);
                        p_c_var_estimator_temp->addDataPoint(current_temperature);

                        std::cout << "std of cy: " << sqrt(p_c_var_estimator_cy->getVariance()) << std::endl;
                        std::cout << "std of temperature:" << sqrt(p_c_var_estimator_temp->getVariance()) << std::endl;
                        std::cout << " +++++++++++++++++++++++++++++++++++++++++++ " << std::endl;
                        std::cout << std::endl;
                        std::cout << std::endl;
                    } // valid?
                    else {
                        std::cout<<"ERROR: No enough data for pars estimation." << std::endl;
                    }

                    for (int i = 0; i < n_mean_shift_samples; i++) { // check each cy-test resut

                        // average of fillrate (accmunated fillrate for mean-shift)
                        if ((pars.e_estimator_type == E_ESTIMATOR_TYPES::_mean_shift) && (n_center_cy_no_change > 1)) {
                            p_fill_rates[i] *= (float)n_center_cy_no_change;
                        }
                    }
                }
                else {

                    // sampling
                    float fill_rate = 0;
                    if ((pars.n_cols > 0) && (pars.blind_zone_ratio > 0)) {
                        fill_rate = calculateNonZeroRatio(p_depth, n_depth_pixels, pars.n_cols, pars.n_rows, pars.blind_zone_ratio,
                                                          pars.b_random_sampling, sampling_roi.sx, sampling_roi.sy, sampling_roi.width, sampling_roi.height
                        );
                    }
                    else
                        fill_rate = calculateNonZeroRatio(p_depth, n_depth_pixels);

                    // get current cy value in IC
                    float value = Get_HW_Reg_2Bytes_Data(pars.reg_addrs_cy, pars.magnification_for_setting_register_cy);
                    if (i_order == 0) { // test for new set
                        center_cy = value; // 1st test is the center
                        oputs.comp_cy = center_cy;
                        period_for_running_oc_section = pars.cy_sampling_period_in_seconds; // set sampling period


                        if (center_cy == last_center_cy)
                            n_center_cy_no_change++;
                        else
                            n_center_cy_no_change = 1;

                    }

                    p_test_cys[i_order] = value;

                    if (pars.e_estimator_type == E_ESTIMATOR_TYPES::_mean_shift) {
                        //if (accmulated_cnt > 0) { // cy not updated --> acculamate data
                        if (center_cy == last_center_cy) { // if center cy not changed, accmulating confidence
                            p_fill_rates[i_order] += fill_rate;
                        }
                        else {  // new cy
                            p_fill_rates[i_order] = fill_rate;
                        }
                    }
                    else if (pars.e_estimator_type == E_ESTIMATOR_TYPES::_confidence_weighted_ranking) {
                        p_fill_rates[i_order] = fill_rate;
                    }


                    if ((i_order == 0) && (fill_rate > pars.valid_min_fill_rate_threshold)) {
                        double dataPoint = fill_rate;
                        p_c_var_estimator_fill_rate->addDataPoint(dataPoint);
                    }

                    float next_test_cy = center_cy + p_increments[i_order] * pars.increment_step_in_cy_test;
                    std::cout << "center_cy : " << center_cy << ", test_cy:" << p_test_cys[i_order] << ", fill_rate:" << fill_rate << ", next_test_cy:" << next_test_cy << std::endl;
                    Set_HW_Reg_2Bytes_Data(pars.reg_addrs_cy, next_test_cy, pars.magnification_for_setting_register_cy);

                    i_order++;
                }
            }
        } // cout << "do_temp_compensation: i_order" << i_order << endl;
    } // if (period_tmp_oc > period_for_running_oc_section) {
    else {
        info.e_states = E_STATES::_idle;
    }

    return (int)E_ERROR::_OK;
}

float Get_HW_Reg_2Bytes_Data(unsigned short addrs[2], int fractional) {
    unsigned short addr_low = addrs[0];
    unsigned short value_low = 0;
    APC_GetHWRegister(g_pHandleApcDI, g_pDevSelInfo, addr_low, &value_low, FG_Address_2Byte | FG_Value_1Byte);

    unsigned short addr_high = addrs[1];
    unsigned short value_high = 0;
    APC_GetHWRegister(g_pHandleApcDI, g_pDevSelInfo, addr_high, &value_high, FG_Address_2Byte | FG_Value_1Byte);

    return (float)(value_low | (value_high << 8)) / fractional;
}

float Get_HW_Reg_3Bytes_Data(unsigned short addrs[3], int fractional){
    unsigned short addr_low = addrs[0];
    unsigned short value_low = 0;
    APC_GetHWRegister(g_pHandleApcDI, g_pDevSelInfo, addr_low, &value_low, FG_Address_2Byte | FG_Value_1Byte);

    unsigned short addr_med = addrs[1];
    unsigned short value_med = 0;
    APC_GetHWRegister(g_pHandleApcDI, g_pDevSelInfo, addr_med, &value_med, FG_Address_2Byte | FG_Value_1Byte);

    unsigned short addr_high = addrs[2];
    unsigned short value_high = 0;
    APC_GetHWRegister(g_pHandleApcDI, g_pDevSelInfo, addr_high, &value_high, FG_Address_2Byte | FG_Value_1Byte);

    return (float)((unsigned int)value_low | (((unsigned int)value_med) << 8) | (((unsigned int)value_high) << 16)) / fractional;
}
int Set_HW_Reg_2Bytes_Data(unsigned short addrs[2], float value, int fractional){
    float value_after_the_fractional = value * fractional;
    unsigned short int_value_after_the_fractional = round(value_after_the_fractional);

    unsigned short addr_low = addrs[0];
    unsigned short value_low = int_value_after_the_fractional & 0x00ff;
    APC_SetHWRegister(g_pHandleApcDI, g_pDevSelInfo, addr_low, value_low, FG_Address_2Byte | FG_Value_1Byte);

    unsigned short addr_high = addrs[1];
    unsigned short value_high = (int_value_after_the_fractional & 0xff00) >> 8;
    APC_SetHWRegister(g_pHandleApcDI, g_pDevSelInfo, addr_high, value_high, FG_Address_2Byte | FG_Value_1Byte);

    return 0;
}

int Set_HW_Reg_3Bytes_Data(unsigned short addrs[3], float value, int fractional){
    float value_after_the_fractional = value * fractional;
    unsigned int int_value_after_the_fractional = round(value_after_the_fractional);

    unsigned short addr_low = addrs[0];
    unsigned short value_low = int_value_after_the_fractional & 0x0000ff;
    APC_SetHWRegister(g_pHandleApcDI, g_pDevSelInfo, addr_low, value_low, FG_Address_2Byte | FG_Value_1Byte);

    unsigned short addr_med = addrs[1];
    unsigned short value_med = (int_value_after_the_fractional & 0x00ff00) >> 8;
    APC_SetHWRegister(g_pHandleApcDI, g_pDevSelInfo, addr_med, value_med, FG_Address_2Byte | FG_Value_1Byte);

    unsigned short addr_high = addrs[2];
    unsigned short value_high = (int_value_after_the_fractional & 0xff0000) >> 16;
    APC_SetHWRegister(g_pHandleApcDI, g_pDevSelInfo, addr_high, value_high, FG_Address_2Byte | FG_Value_1Byte);

    return 0;
}

SelfK2::ROI SelfK2::getRandomROI(int n_cols, int n_rows, double rat_x, double rat_y) {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<int> dist_x(0, n_cols - 1);
    std::uniform_int_distribution<int> dist_y(0, n_rows - 1);

    int min_width = n_cols * rat_x;
    int min_height = n_rows * rat_y;

    int width = dist_x(gen);
    int height = dist_y(gen);

    // Adjust width and height to fit within image bounds
    width = std::max(width, min_width);
    height = std::max(height, min_height);

    width = std::min(width, n_cols);
    height = std::min(height, n_rows);

    int sx = dist_x(gen);
    int sy = dist_y(gen);

    // Adjust sx and sy to ensure ROI is within image bounds
    sx = std::max(0, sx);
    sy = std::max(0, sy);
    width = std::min(width, n_cols - sx);
    height = std::min(height, n_rows - sy);

    return { sx, sy, width, height };
}

void CSelfCalibration2Controller::updateStatusText()
{
    std::string text;
    text.append("Temperature: " + std::to_string(GetCurrentSensorTemperature()) + "\n");

    if (m_cy_compensator) {
        text.append("\n[Outputs]\n");
        text.append("comp_cy: " + std::to_string(m_cy_compensator->oputs.comp_cy) + "\n");
        text.append("default_cy: " + std::to_string(m_cy_compensator->oputs.default_cy) + "\n");
        text.append("estimated_cy: " + std::to_string(m_cy_compensator->oputs.estimaed_cy) + "\n");
        text.append("default_cy_in_register: " + std::to_string(m_cy_compensator->oputs.default_cy_in_register) + "\n");

        text.append("[Info]\n");
        text.append("p_f_in_acquisition: " + std::to_string(*m_cy_compensator->info.p_f_in_acquisition) + "\n");
        text.append("p_n_acq_iterations: " + std::to_string(*m_cy_compensator->info.p_n_acq_iterations) + "\n");
        text.append("p_convergency_cnt: " + std::to_string(*m_cy_compensator->info.p_convergency_cnt) + "\n");
        text.append("p_i_test: " + std::to_string(*m_cy_compensator->info.p_i_test) + "\n");
        text.append("p_i_successfully_test: " + std::to_string(*m_cy_compensator->info.p_i_successfully_test) + "\n");
        text.append("p_i_update_pars: " + std::to_string(*m_cy_compensator->info.p_i_update_pars) + "\n");

        text.append("v_test_cys & fillrates:\n");
        int n_test_cys = m_cy_compensator->info.v_test_cys.size();
        for (int i = 0; i < n_test_cys; i++) {
            text.append(std::to_string(m_cy_compensator->info.v_test_cys[i]) + ", " +
                        std::to_string(m_cy_compensator->info.v_fill_rate_of_cys[i]) + "\t");
            if (i % 2 == 1) text.append("\n");
        }

        text.append("\np_convergency_cnt: " + std::to_string(*m_cy_compensator->info.p_convergency_cnt) + "\n");
    }

    m_statusText = text;
}

void CSelfCalibration2Controller::compensatorThread() {
    std::stringstream ss;
    ss << std::this_thread::get_id();

    bool cy_compensator_is_idle = false;

    while (m_isRunning.load()) {
        if (!IsDepthStreaming()) {
            m_isRunning.store(false);
            break;
        }

        if (!GetCompensatorWorking()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_PERIOD_IN_MS));
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(m_CompensatorMutex);

            if (m_Reset) {
                if (m_cy_compensator) {
                    m_cy_compensator->_reset();
                }
                if (m_focal_compensator) {
                    m_focal_compensator->_reset();
                }
                m_Reset = false;
                continue;
            }
            //UpdateDepthImage();
            if (m_cy_compensator && m_Depth.size() > 0) {
                float temperature = GetCurrentSensorTemperature();
                int ret;
                {
                    std::lock_guard<std::mutex> depthLock(mDepthMutex);
                    ret = m_cy_compensator->run(&m_Depth[0],
                                                m_cy_compensator->pars.n_cols * m_cy_compensator->pars.n_rows,
                                                temperature);
                }
                
                if (ret < 0) {
                    std::cout << "[error] cy_compensator:" << m_cy_compensator->map_error_code[ret];
                }

                cy_compensator_is_idle = (m_cy_compensator->info.e_states == SelfK2::C_Cy_Compensator::E_STATES::_idle);

                // Update status text
                updateStatusText();
            }

            if (m_focal_compensator && cy_compensator_is_idle) {
                float temperature = GetCurrentSensorTemperature();
                int ret = m_focal_compensator->run(temperature);
                if (ret < 0) {
                    std::cout << "[error] focal_compensator:" << m_focal_compensator->map_error_code[ret];
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_PERIOD_IN_MS));
    }
}

void CSelfCalibration2Controller::stopThreads() {
    m_isRunning.store(false);
    std::cout << "[stopThreads] Waiting 1.2s for threads to notice stop signal...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    if (m_temperatureThread && m_temperatureThread->joinable()) {
        m_temperatureThread->join();
        m_temperatureThread.reset();
    }

    if (m_compensatorThread && m_compensatorThread->joinable()) {
        m_compensatorThread->join();
        m_compensatorThread.reset();
    }

    SelfK2::set_hw_handler(nullptr, nullptr, nullptr);
}

void CSelfCalibration2Controller::UpdateRectifyLogData(eSPCtrl_RectLogData &data) {
    if (m_RectifyData == nullptr) {
        return;
    }

    memcpy(m_RectifyData.get(), &data, sizeof(eSPCtrl_RectLogData));
}

float CSelfCalibration2Controller::GetCurrentCompCy() {
    if (m_cy_compensator) {
        return m_cy_compensator->oputs.comp_cy;
    }
    fprintf(stderr, "Set default Cy %f\n", m_cy_compensator->oputs.default_cy_in_register);
    return m_cy_compensator->oputs.default_cy_in_register;
}

void CSelfCalibration2Controller::SetCompensatorWorking(bool shouldWorking) {
    mCompensatorWorking = shouldWorking;
}
bool CSelfCalibration2Controller::GetCompensatorWorking(void) {
    return mCompensatorWorking;
}
