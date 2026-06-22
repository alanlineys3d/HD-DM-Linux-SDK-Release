#include <eSPDI_def.h>
#include "ModeConfig.h"
#include "sqlite3.h"
#include <sstream>

#define DEVICE_TABLE_COLUMN             4
#define DEVICE_TABLE_IDX_PID            1
#define DEVICE_TABLE_IDX_TABLE_NAME     2
#define DEVICE_TABLE_IDX_IMU_TYPE       3

#define PID_TABLE_COLUMN                15
#define PID_SUPPORT_TABLE_COLUMN_v1     12
#define PID_SUPPORT_TABLE_COLUMN_v2     15
#define PID_TABLE_COLUMN_GRAPE          24
#define PID_TABLE_COLUMN_GRAPE_V2       26  // Grape v2 with Video_Mode_D11_ColorOnly and Video_Mode_Z14

#define PID_TABLE_IDX_MODE              0
#define PID_TABLE_IDX_MODE_DESC         1
#define PID_TABLE_IDX_L_RESOLUTION      2
#define PID_TABLE_IDX_D_RESOLUTION      3
#define PID_TABLE_IDX_K_RESOLUTION      4
#define PID_TABLE_IDX_T_RESOLUTION      5
#define PID_TABLE_IDX_DEPTH_TYPE        6
#define PID_TABLE_IDX_COLOR_FPS         7
#define PID_TABLE_IDX_DEPTH_FPS         8
#define PID_TABLE_IDX_USB_TYPE          9
#define PID_TABLE_IDX_RECTIFY_MODE      10
#define PID_TABLE_IDX_INTER_LEAVE_MODE  11
#define PID_TABLE_IDX_VIDEO_MODE_D11_OR_COLOR_ONLY  12
#define PID_TABLE_IDX_VIDEO_MODE_Z14                13
#define PID_TABLE_IDX_RECTIFY_FILE_INDE             14

#define PID_TABLE_IDX_L_DATA_TYPE       12
#define PID_TABLE_IDX_K_DATA_TYPE       13
#define PID_TABLE_IDX_T_DATA_TYPE       14
#define PID_TABLE_IDX_L_VIDEO_MODE      15
#define PID_TABLE_IDX_K_VIDEO_MODE      16
#define PID_TABLE_IDX_T_VIDEO_MODE      17
#define PID_TABLE_IDX_L_FPS             18
#define PID_TABLE_IDX_K_FPS             19
#define PID_TABLE_IDX_T_FPS             20
#define PID_TABLE_IDX_L_INDEX           21
#define PID_TABLE_IDX_K_INDEX           22
#define PID_TABLE_IDX_T_INDEX           23
// Grape v2 table added (replace K_VideoMode functionality)
#define PID_TABLE_IDX_GRAPE_VIDEO_MODE_D11_OR_COLOR_ONLY  24
#define PID_TABLE_IDX_GRAPE_VIDEO_MODE_Z14                25

ModeConfig& g_ModeConfig = ModeConfig::GetModeConfig();

ModeConfig ModeConfig::m_ModeConfig;

ModeConfig::ModeConfig() : m_sq3( nullptr )
{
    ReadModeConfig();
}

ModeConfig::~ModeConfig()
{
    if ( m_sq3 ) sqlite3_close( m_sq3 );
}

const std::vector< ModeConfig::MODE_CONFIG >& ModeConfig::GetModeConfigList( const int iPID )
{
    static std::vector< ModeConfig::MODE_CONFIG > empty;

    if ( m_mapDeviceTable.find( iPID ) != m_mapDeviceTable.end() )
    {
        return m_mapDeviceTable[ iPID ].vecModeConfig;
    }
    return empty;
}

ModeConfig::IMU_TYPE ModeConfig::GetIMU_Type( const int iPID )
{
    if ( m_mapDeviceTable.find( iPID ) != m_mapDeviceTable.end() )
    {
        return m_mapDeviceTable[ iPID ].IMU_Type;
    }
    return IMU_NONE;
}

bool ModeConfig::IsSupportedColumnNumber(int counts) {
    switch (counts)
    {
        case PID_SUPPORT_TABLE_COLUMN_v1:
        case PID_SUPPORT_TABLE_COLUMN_v2:
        case PID_TABLE_COLUMN_GRAPE:
        case PID_TABLE_COLUMN_GRAPE_V2:
            return true;
        default:
            return false;
    }
}

void ModeConfig::ReadModeConfig()
{
    if ( SQLITE_OK != sqlite3_open_v2( "../cfg/ModeConfig.db", &m_sq3, SQLITE_OPEN_READONLY, NULL ) )
    {
        return;
    }
    sqlite3_stmt *stmt = nullptr;

    int PID = 0;

    char szDecodeType[ 16 ] = { 0 };

    if ( SQLITE_OK != sqlite3_prepare_v2( m_sq3, "Select * from [DeviceTable]", EOF, &stmt, NULL ) )
    {
        return;
    }
    while ( SQLITE_ROW == sqlite3_step( stmt ) )
    {
        if ( DEVICE_TABLE_COLUMN != sqlite3_column_count( stmt ) )
        {
            continue;
        }
        if ( SQLITE_TEXT == sqlite3_column_type( stmt, DEVICE_TABLE_IDX_PID ) )
        {
            const char * valChar = ( char* )sqlite3_column_text(stmt, DEVICE_TABLE_IDX_PID );

            PID = valChar ? strtoul( valChar, NULL, 16 ) : 0;
        }
        if ( SQLITE_TEXT == sqlite3_column_type( stmt, DEVICE_TABLE_IDX_TABLE_NAME ) )
        {
            m_mapDeviceTable[ PID ].csTableName = QString::fromLocal8Bit( ( char* )sqlite3_column_text(stmt, DEVICE_TABLE_IDX_TABLE_NAME ));
        }
        if ( SQLITE_INTEGER == sqlite3_column_type( stmt, DEVICE_TABLE_IDX_IMU_TYPE ) )
        {
            m_mapDeviceTable[ PID ].IMU_Type = ( IMU_TYPE )sqlite3_column_int(stmt, DEVICE_TABLE_IDX_IMU_TYPE );
        }
    }
    sqlite3_finalize( stmt );

    QString csCommand;

    auto ResolutionParse = [ & ] ( const int iSqlIndex, MODE_CONFIG::RESOLUTION& xResolution, MODE_CONFIG::DECODE_TYPE& DecodeType )
    {
        sscanf( ( char* )sqlite3_column_text( stmt, iSqlIndex ), "%dx%d_%s", &xResolution.Width, &xResolution.Height, szDecodeType );

        if ( !strcmp( "MJPEG", szDecodeType ) ) DecodeType = MODE_CONFIG::MJPEG;
    };
    for ( auto& Table : m_mapDeviceTable )
    {
        csCommand.sprintf( "Select * from [%s]", Table.second.csTableName.toLocal8Bit().data() );

        if ( SQLITE_OK != sqlite3_prepare_v2( m_sq3, csCommand.toLocal8Bit().data(), EOF, &stmt, NULL ) )
        {
            continue;
        }
        while ( SQLITE_ROW == sqlite3_step( stmt ) )
        {
            //if ( PID_TABLE_COLUMN != sqlite3_column_count( stmt ) )
            int columnCounts = sqlite3_column_count( stmt );
            
            if ( !IsSupportedColumnNumber(columnCounts) )
            {
                continue;
            }
            MODE_CONFIG xModeConfig;

            if ( SQLITE_INTEGER == sqlite3_column_type( stmt, PID_TABLE_IDX_MODE ) )
            {
                xModeConfig.iMode = sqlite3_column_int(stmt, PID_TABLE_IDX_MODE );
            }
            if ( SQLITE_TEXT == sqlite3_column_type( stmt, PID_TABLE_IDX_MODE_DESC ) )
            {
                xModeConfig.csModeDesc = ( char* )sqlite3_column_text( stmt, PID_TABLE_IDX_MODE_DESC );
            }
            if ( SQLITE_TEXT == sqlite3_column_type( stmt, PID_TABLE_IDX_L_RESOLUTION ) )
            {
                ResolutionParse( PID_TABLE_IDX_L_RESOLUTION, xModeConfig.L_Resolution, xModeConfig.eDecodeType_L );
            }
            if ( SQLITE_TEXT == sqlite3_column_type( stmt, PID_TABLE_IDX_D_RESOLUTION ) )
            {
                sscanf( ( char* )sqlite3_column_text( stmt, PID_TABLE_IDX_D_RESOLUTION ), "%dx%d", &xModeConfig.D_Resolution.Width, &xModeConfig.D_Resolution.Height );
            }
            if ( SQLITE_TEXT == sqlite3_column_type( stmt, PID_TABLE_IDX_K_RESOLUTION ) )
            {
                ResolutionParse( PID_TABLE_IDX_K_RESOLUTION, xModeConfig.K_Resolution, xModeConfig.eDecodeType_K );
            }
            if ( SQLITE_TEXT == sqlite3_column_type( stmt, PID_TABLE_IDX_T_RESOLUTION ) )
            {
                ResolutionParse( PID_TABLE_IDX_T_RESOLUTION, xModeConfig.T_Resolution, xModeConfig.eDecodeType_T );
            }
            if ( SQLITE_TEXT == sqlite3_column_type( stmt, PID_TABLE_IDX_DEPTH_TYPE ) )
            {
                std::stringstream ssDepthType( ( char* )sqlite3_column_text( stmt, PID_TABLE_IDX_DEPTH_TYPE ) );

                for ( std::string each; std::getline( ssDepthType, each, ',' ); xModeConfig.vecDepthType.push_back( atoi( each.c_str() ) ) );
            }
            if ( SQLITE_TEXT == sqlite3_column_type( stmt, PID_TABLE_IDX_COLOR_FPS ) )
            {
                std::stringstream ssFrameRate( ( char* )sqlite3_column_text( stmt, PID_TABLE_IDX_COLOR_FPS ) );

                for ( std::string each; std::getline( ssFrameRate, each, ',' ); xModeConfig.vecColorFps.push_back( atoi( each.c_str() ) ) );
            }
            if ( SQLITE_TEXT == sqlite3_column_type( stmt, PID_TABLE_IDX_DEPTH_FPS ) )
            {
                std::stringstream ssFrameRate( ( char* )sqlite3_column_text( stmt, PID_TABLE_IDX_DEPTH_FPS ) );

                for ( std::string each; std::getline( ssFrameRate, each, ',' ); xModeConfig.vecDepthFps.push_back( atoi( each.c_str() ) ) );
            }
            if ( SQLITE_INTEGER == sqlite3_column_type( stmt, PID_TABLE_IDX_USB_TYPE ) )
            {
                xModeConfig.iUSB_Type = sqlite3_column_int(stmt, PID_TABLE_IDX_USB_TYPE );
            }
            if ( SQLITE_INTEGER == sqlite3_column_type( stmt, PID_TABLE_IDX_RECTIFY_MODE ) )
            {
                xModeConfig.bRectifyMode = ( sqlite3_column_int(stmt, PID_TABLE_IDX_RECTIFY_MODE ) > 0 );
            }
            if ( SQLITE_INTEGER == sqlite3_column_type( stmt, PID_TABLE_IDX_INTER_LEAVE_MODE ) )
            {
                xModeConfig.iInterLeaveModeFPS = sqlite3_column_int(stmt, PID_TABLE_IDX_INTER_LEAVE_MODE );
            }
            if (Table.first == APC_PID_GRAPE || Table.first == APC_PID_80363IR || Table.first == APC_PID_80363C)
            {
                // start - used by grape
                if (SQLITE_INTEGER == sqlite3_column_type(stmt, PID_TABLE_IDX_L_DATA_TYPE))
                {
                    xModeConfig.iL_DataType = sqlite3_column_int(stmt, PID_TABLE_IDX_L_DATA_TYPE);
                }
                if (SQLITE_INTEGER == sqlite3_column_type(stmt, PID_TABLE_IDX_K_DATA_TYPE))
                {
                    xModeConfig.iK_DataType = sqlite3_column_int(stmt, PID_TABLE_IDX_K_DATA_TYPE);
                }
                if (SQLITE_INTEGER == sqlite3_column_type(stmt, PID_TABLE_IDX_T_DATA_TYPE))
                {
                    xModeConfig.iT_DataType = sqlite3_column_int(stmt, PID_TABLE_IDX_T_DATA_TYPE);
                }
                if (SQLITE_INTEGER == sqlite3_column_type(stmt, PID_TABLE_IDX_L_VIDEO_MODE))
                {
                    xModeConfig.iL_VideoMode = sqlite3_column_int(stmt, PID_TABLE_IDX_L_VIDEO_MODE);
                }
                if (SQLITE_INTEGER == sqlite3_column_type(stmt, PID_TABLE_IDX_K_VIDEO_MODE))
                {
                    xModeConfig.iK_VideoMode = sqlite3_column_int(stmt, PID_TABLE_IDX_K_VIDEO_MODE);
                }
                if (SQLITE_INTEGER == sqlite3_column_type(stmt, PID_TABLE_IDX_T_VIDEO_MODE))
                {
                    xModeConfig.iT_VideoMode = sqlite3_column_int(stmt, PID_TABLE_IDX_T_VIDEO_MODE);
                }
                if ( SQLITE_INTEGER == sqlite3_column_type( stmt, PID_TABLE_IDX_T_VIDEO_MODE ) )
                {
                    xModeConfig.iT_VideoMode = sqlite3_column_int(stmt, PID_TABLE_IDX_T_VIDEO_MODE );
                }
                if ( SQLITE_INTEGER == sqlite3_column_type( stmt, PID_TABLE_IDX_L_FPS ) )
                {
                    xModeConfig.iL_FPS = sqlite3_column_int(stmt, PID_TABLE_IDX_L_FPS );
                }
                if ( SQLITE_INTEGER == sqlite3_column_type( stmt, PID_TABLE_IDX_K_FPS ) )
                {
                    xModeConfig.iK_FPS = sqlite3_column_int(stmt, PID_TABLE_IDX_K_FPS );
                }
                if ( SQLITE_INTEGER == sqlite3_column_type( stmt, PID_TABLE_IDX_T_FPS ) )
                {
                    xModeConfig.iT_FPS = sqlite3_column_int(stmt, PID_TABLE_IDX_T_FPS );
                }
                if ( SQLITE_INTEGER == sqlite3_column_type( stmt, PID_TABLE_IDX_L_INDEX ) )
                {
                    xModeConfig.iL_Index = sqlite3_column_int(stmt, PID_TABLE_IDX_L_INDEX );
                }
                if ( SQLITE_INTEGER == sqlite3_column_type( stmt, PID_TABLE_IDX_K_INDEX ) )
                {
                    xModeConfig.iK_Index = sqlite3_column_int(stmt, PID_TABLE_IDX_K_INDEX );
                }
                if ( SQLITE_TEXT == sqlite3_column_type( stmt, PID_TABLE_IDX_T_INDEX ) )
                {
                    std::stringstream ssDepthIndex((char*)sqlite3_column_text(stmt, PID_TABLE_IDX_T_INDEX));
                    for (std::string each; std::getline(ssDepthIndex, each, ',');
                         xModeConfig.iT_Index.push_back(atoi(each.c_str())));
                }
                // Grape v2 table: read Video_Mode_D11_ColorOnly and Video_Mode_Z14 (replaces K_VideoMode)
                if (columnCounts >= PID_TABLE_COLUMN_GRAPE_V2 && SQLITE_INTEGER == sqlite3_column_type(stmt, PID_TABLE_IDX_GRAPE_VIDEO_MODE_D11_OR_COLOR_ONLY))
                {
                    xModeConfig.videoModeD11OrColorOnly = sqlite3_column_int(stmt, PID_TABLE_IDX_GRAPE_VIDEO_MODE_D11_OR_COLOR_ONLY);
                }
                if (columnCounts >= PID_TABLE_COLUMN_GRAPE_V2 && SQLITE_INTEGER == sqlite3_column_type(stmt, PID_TABLE_IDX_GRAPE_VIDEO_MODE_Z14))
                {
                    xModeConfig.videoModeZ14 = sqlite3_column_int(stmt, PID_TABLE_IDX_GRAPE_VIDEO_MODE_Z14);
                }
                // end - used by grape
            }
            else
            {
                if (columnCounts >= PID_SUPPORT_TABLE_COLUMN_v2 && SQLITE_INTEGER == sqlite3_column_type(stmt, PID_TABLE_IDX_VIDEO_MODE_D11_OR_COLOR_ONLY))
                {
                    xModeConfig.videoModeD11OrColorOnly = sqlite3_column_int(stmt, PID_TABLE_IDX_VIDEO_MODE_D11_OR_COLOR_ONLY);
                }
                if (columnCounts >= PID_SUPPORT_TABLE_COLUMN_v2 && SQLITE_INTEGER == sqlite3_column_type(stmt, PID_TABLE_IDX_VIDEO_MODE_Z14))
                {
                    xModeConfig.videoModeZ14 = sqlite3_column_int(stmt, PID_TABLE_IDX_VIDEO_MODE_Z14);
                }
                if (columnCounts >= PID_SUPPORT_TABLE_COLUMN_v2 && SQLITE_INTEGER == sqlite3_column_type(stmt, PID_TABLE_IDX_RECTIFY_FILE_INDE))
                {
                    xModeConfig.rectifyFileIndex = sqlite3_column_int(stmt, PID_TABLE_IDX_RECTIFY_FILE_INDE);
                }
            }
            Table.second.vecModeConfig.push_back( std::move( xModeConfig ) );
        }
        sqlite3_finalize( stmt );
    }
}

QString ModeConfig::MODE_CONFIG::ToString() const
{
    QString result;
    QTextStream stream(&result);
    
    stream << "Mode: " << iMode << "\n"
           << "Mode Description: " << csModeDesc << "\n"
           << "USB Type: " << iUSB_Type << "\n"
           << "InterLeave Mode FPS: " << iInterLeaveModeFPS << "\n"
           << "Rectify Mode: " << (bRectifyMode ? "Yes" : "No") << "\n"
           << "Decode Type (L/K/T): " << eDecodeType_L << "/" << eDecodeType_K << "/" << eDecodeType_T << "\n"
           << "L Resolution - Display: " << L_ResolutionConfig.displayResolution.Width << "x" << L_ResolutionConfig.displayResolution.Height 
           << " Device: " << L_ResolutionConfig.deviceOpeningResolution.Width << "x" << L_ResolutionConfig.deviceOpeningResolution.Height << "\n"
           << "D Resolution: " << D_Resolution.Width << "x" << D_Resolution.Height << "\n"
           << "K Resolution - Display: " << K_ResolutionConfig.displayResolution.Width << "x" << K_ResolutionConfig.displayResolution.Height 
           << " Device: " << K_ResolutionConfig.deviceOpeningResolution.Width << "x" << K_ResolutionConfig.deviceOpeningResolution.Height << "\n"
           << "T Resolution - Display: " << T_ResolutionConfig.displayResolution.Width << "x" << T_ResolutionConfig.displayResolution.Height 
           << " Device: " << T_ResolutionConfig.deviceOpeningResolution.Width << "x" << T_ResolutionConfig.deviceOpeningResolution.Height << "\n"
           << "Depth Types: ";
    
    for(int type : vecDepthType) stream << type << " ";
    stream << "\nColor FPS: ";
    for(int fps : vecColorFps) stream << fps << " ";
    stream << "\nDepth FPS: ";
    for(int fps : vecDepthFps) stream << fps << " ";
    
    if(videoModeD11OrColorOnly != 0xff)
        stream << "\nVideo Mode D11/Color Only: 0x" << QString::number(videoModeD11OrColorOnly, 16);
    if(videoModeZ14 != 0xff)
        stream << "\nVideo Mode Z14: 0x" << QString::number(videoModeZ14, 16);
    if(rectifyFileIndex != 0xff)
        stream << "\nRectify File Index: " << rectifyFileIndex;
        
    // GRAPE specific
    stream << "\nData Types (L/K/T): " << iL_DataType << "/" << iK_DataType << "/" << iT_DataType
           << "\nVideo Modes (L/K/T): " << iL_VideoMode << "/" << iK_VideoMode << "/" << iT_VideoMode
           << "\nFPS (L/K/T): " << iL_FPS << "/" << iK_FPS << "/" << iT_FPS
           << "\nIndices (L/K): " << iL_Index << "/" << iK_Index;
    
    stream << "\nT Indices: ";
    for(int index : iT_Index) stream << index << " ";

    stream << "\n";

    return result;
}