#ifndef __COMMON_H__
#define __COMMON_H__

#include <QString>

#define _TONER_CHIP_ADDR    0x2c
#define _DRUM_CHIP_ADDR     0x28

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

//上位机发送给下位机的命令
enum {
    OP_WRITE_INFO,           //上位机写入耗材信息
    OP_READ_TONER_INFO,      //上位机读取耗材信息
    OP_READ_DRUM_INFO,
    OP_GET_STATUS,          //获取治具状态，返回治具上安装的耗材芯片状态
    OP_GET_SUPPLY_INFO,     //获取耗材信息

    OP_GET_STATE_LONGCONN,  //获取状态的长连接
    OP_WRITE_BULK_INFO,     //批量写入耗材信息
    OP_SEND_BULK_INFO,      //发送批量数据到治具
    FIXTURE_TRIGGER_UP,         //抬起动作
    RE_HEARTBEAT_SIGNAL,    //发送心跳包，维护长连接句柄有效性

    OP_BROADCAST_UDP_REQUEST = 99,
    OP_BROADCAST_UDP_RESP,
    OP_HB_UDP_REQUEST,       //UDP的心跳包
};

//下位机的反馈
enum {
    RESP_OK,
    RESP_ERROR,
    RESP_NO_DRUM,
    RESP_NO_TONER,

    RESP_ALL_DETECT   = 0x10,         //安装粉盒和鼓组件
    RESP_NO_DRUM_DETECT  = 0x11,      //未安装鼓组件
    RESP_NO_TONER_DETECT = 0x12,      //为未安装粉盒
    RESP_NO_DETECT   = 0x13,          //未安装鼓组件和粉盒
};

typedef struct cgmsghdr {
    int cmd;         //命令码
    int i2c_addr;    //i2c地址，根据耗材型号修改
                     //DL开头为0x28，TL开头为0x2c
    int len;         //数据长度，不包含头部
} __attribute__((__packed__)) MsgHdr;

typedef struct respinfo {
    int cmd;
    int ret;
} __attribute__((__packed__)) RespInfo;

// 耗材出厂设置的信息，出厂写入的初始值，该数据存储在一次性写入区域，写入后无法更改
struct cgprintech_supply_info
{
    char model_id[16];              //型号
    char serial_no[32];             //序列号

    char marketing_area[4];         //销售地区，中国为"CN"，美国为"US"，日本为"JP"
    uint8_t product_date[4];        //出厂日期，如2022年7月23日，这里应为0x20,0x22,0x07,0x23

    char manufacturer[16];          //厂商名称，如"cgprintech"
    char trade_mark[16];            //商标，如"cgprintech"
    char type[4];                   //类型，原装为"I"，授权贩卖为"M"

    uint8_t pages[4];        //总页数
    uint8_t dots[4];         //总点数

    //允许超出打印数量，页数/百分比
    uint8_t beyond_pages[2];     //溢出打印的页数，填实际页数
    uint8_t beyond_percent[2];   //溢出打印比例，如比例为10%，则这里填10
                                 //耗材总容量为5000页，耗尽后，还将允许打印5000*5%页

    uint8_t free_pages[2];       //前N页打印不计数消耗页数
} __attribute__((__packed__));
#define CGPRINTECH_SUPPLY_INFO_LEN   sizeof(struct cgprintech_supply_info)

//从数据库读取的耗材信息
struct cgprintech_supply_sqlinfo
{
    QString operator_id;             //操作员
    QString foreman;         //领班
    char model_id[16];              //型号
    char serial_no[32];             //序列号

    char marketing_area[4];         //销售地区，中国为"CN"，美国为"US"，日本为"JP"
    char product_date[12];
    char manufacturer[16];          //厂商名称，如"cgprintech"
    char trade_mark[16];            //商标，如"cgprintech"
    char type[4];                   //类型，原装为"O"，授权贩卖为"M"

    char pages[12];
    char dots[12];
    char overflow_pages[4];
    char overflow_percent[4];
    char free_pages[4];
};
#define SUPPLY_SQLINFO_LEN  sizeof(struct cgprintech_supply_sqlinfo)

//读取耗材信息
struct cgprintech_supply_info_readback
{
    char model_id[16];              //型号
    char serial_no[32];             //序列号

    char marketing_area[4];         //销售地区，中国为"CN"，美国为"US"，日本为"JP"
    uint8_t product_date[4];  //出厂日期，如2022年7月23日，这里应为0x20,0x22,0x07,0x23

    char manufacturer[16];          //厂商名称，如"cgprintech"
    char trade_mark[16];            //商标，如"cgprintech"
    char type[4];                   //类型，原装为"I"，授权贩卖为"M"

    uint8_t pages[4];        //总页数
    uint8_t dots[4];         //总点数

    uint8_t beyond_pages[2];    //溢出打印的页数，填实际页数
    uint8_t beyond_percent[2];  //溢出打印比例，如比例为10%，则这里填10
                                //耗材总容量为5000页，耗尽后，还将允许打印5000*5%页
    uint8_t free_pages[2];      //前N页打印不计数消耗页数

    uint8_t install_date[8];    //第一次安装日期，出厂时为0x00
                                //当免费打印的页数耗尽后，开始消耗正常页数时开始写入
    uint8_t recent_date[8];     //最近一次使用日期，出厂时为0x00，
                                //当免费打印的页数耗尽后，开始消耗正常页数时开始写入
    uint8_t printed_pages[4];   //已打印页数
    uint8_t printed_dots[4];    //已打印点数

    uint8_t print_beyond_pages[2];  //打印超出页数
    uint8_t print_free_pages[2];    //打印免费页数
} __attribute__((__packed__));
#define SUPPLY_INFO_READBACK_LEN    sizeof(struct cgprintech_supply_info_readback)

//上送治具上各卡座状态信息及写芯片结果信息
typedef struct booth_state {
    RespInfo resp;
    uint8_t mode;     //根据上位机对卡座组的选择决定

    //写芯片结果定义
#define _CHIP_WRITE_SUCCESS  0    //写入成功
#define _CHIP_WRITE_FAILED   1    //写入失败
#define _CHIP_HAS_DATA       2    //芯片已有数据，未写
#define _CHIP_MISSING        3    //未发现芯片
    uint8_t state[4];             //根据mode决定可用数据
    char serial_no[4][32];          //4组序列号
} __attribute__((__packed__)) BoothState;

//写单个卡座信息
typedef struct write_booth_chip {
    uint8_t index;           //卡座编号
    struct cgprintech_supply_info info;
} __attribute__((__packed__)) WriteBoothInfo;


//读取卡座上耗材信息
typedef struct read_booth_chip {
    RespInfo resp;
    uint8_t index;           //卡座编号
    struct cgprintech_supply_info_readback info;
} __attribute__((__packed__)) ReadBoothInfo;


//该批次数据，批量发送到治具
typedef struct booth_supply_info_w {
    int booth_num;        //本次使用的1拖4，该值为4，本次使用1拖8，该值为8
    char serial_no[4][32];          //8组序列号
    char model_id[16];              //型号
    char marketing_area[4];         //销售地区
    uint8_t product_date[4];        //出厂日期
    char manufacturer[16];          //厂商名称
    char trade_mark[16];            //商标
    char type[4];                   //类型
    uint8_t pages[4];               //总页数
    uint8_t beyond_pages[2];        //溢出打印的页数
    uint8_t free_pages[2];          //前N页打印不计数消耗页数
} __attribute__((__packed__)) BoothSupplyInfoW;

//治具回复的广播报文
typedef struct bc_info_resp {
    RespInfo resp;
    char ipaddr[40];
} __attribute__((__packed__)) BcInfoResp;

#endif
