#ifndef RTL8651_FDB_H
#define RTL8651_FDB_H
#define RTL8651_MAC_NUMBER						6
#define RTL8651_PORT_NUMBER						RTL8651_MAC_NUMBER
#define RTL8651_MII_PORTNUMBER                 			5
#define RTL8651_MII_PORTMASK                    			0x20
#define RTL8651_PHY_NUMBER						5
//#define RTL8651_AGGREGATOR_NUMBER				(RTL8651_PORT_NUMBER+rtl8651_totalExtPortNum)
//#define RTL8651_PSOFFLOAD_RESV_PORT				(RTL8651_PORT_NUMBER+rtl8651_totalExtPortNum+1)	/* port reserved for protocol stack offloading */
#define RTL8651_ALLPORTMASK				((1<<RTL8651_AGGREGATOR_NUMBER)-1)
#define RTL8651_PHYSICALPORTMASK			((1<<RTL8651_MAC_NUMBER)-1)
#define RTL8651_L2TBL_ROW					256
#define RTL8651_L2TBL_COLUMN				4
#define RTL8651_MAX_AGGREGATION_NUM		1
#define RTL8651_VLAN_NUMBER				8
#define RTL865XC_NETINTERFACE_NUMBER		8
#define RTL865XC_MAC_NETINTERFACE_NUMBER		4
#define RTL865XC_PORT_NUMBER				9
#define RTL865XC_VLAN_NUMBER				4096
#define RTL865XC_LAGHASHIDX_NUMBER			8	/* There are 8 hash values in RTL865xC Link Aggregation. */
#define RTL8651_CPU_PORTNUMBER			RTL8651_MAC_NUMBER

#define RTL8651_L2_NUMBER				1024

#define	FDB_TYPE_FWD					0x00
#define	FDB_TYPE_SRCBLK				0x01
#define	FDB_TYPE_DSTBLK				0x02
#define	FDB_TYPE_TRAPCPU				0x03

#define RTL865x_FDB_NUMBER				4

#define CONFIG_RTL865X_SYNC_L2		1

//#define CONFIG_RTL865X_LANPORT_RESTRICTION
/*
Note:
1. Entry property toCpu, srcBlocking destination blocking (memberPortMask) and normal forwarding (Neither toCpu nor srcBlocking) are
    mutually exclusive. Therefore, only 2-bit to represent this situation.
2. All entries are static configuration
3. Unable to process conflict macAddress entry process, return fail when following operation are conflict with current one.
*/
typedef struct rtl865x_filterDbTableEntry_s {
	ether_addr_t	macAddr;
	uint16  l2type;			// see bleow definition
	uint16	process:2,		//0: Normal forwarding, 1: destination blocking 2: source blocking 3: toCpu
			//Management flag
			refCount:8,		//Referenced by other table, such as
			configToAsic:1,	//This entry is configured to ASIC
			asicPos:2,		//The entry position of the ASIC. Since rtl8651 only provides 4-entry. Only 2-bit is required
			SrcBlk:1,
			auth:1,
			nhFlag:1;
	uint16 	vid;				//0: don't care. 1~4094: vid of this entry
	uint32	linkId;			//For WLAN WDS.
		//cfliu: What is link Id? link Id is a virtual port number which represents a WDS link for wireless application.
		//since 8650 uses MII loopback port and 8650B has only 3 ext ports, it is always insufficient to map 8650B ext ports
		//to typical 8 WDS links.

	uint32	memberPortMask;
} rtl865x_filterDbTableEntry_t;

int32 _rtl865x_addFilterDatabaseEntry(uint16 l2Type, uint16 fid, ether_addr_t * macAddr, uint32 type, uint32 portMask, uint8 auth, uint8 SrcBlk);
int32 _rtl865x_addAuthFilterDatabaseEntry(uint16 l2Type, uint16 fid, ether_addr_t * macAddr, uint32 type, uint32 portMask, uint8 auth, uint8 SrcBlk);
int32 _rtl865x_delAuthFilterDatabaseEntry(uint16 l2Type, uint16 fid, ether_addr_t * macAddr);
int32 rtl865x_addAuthFilterDatabaseEntryExtension( uint16 fid, rtl865x_filterDbTableEntry_t * L2entry);
int32 rtl865x_delAuthFilterDatabaseEntry(uint16 l2Type, uint16 fid, ether_addr_t * macAddr);
int32 _rtl865x_delFilterDatabaseEntry(uint16 l2Type, uint16 fid, ether_addr_t * macAddr);
uint32 rtl865x_getHWL2Index(ether_addr_t * macAddr, uint16 fid);
int32 rtl865x_setHWL2Table(uint32 row, uint32 column, rtl865x_tblAsicDrv_l2Param_t *l2p);
int32 rtl865x_getHWL2Table(uint32 row, uint32 column, rtl865x_tblAsicDrv_l2Param_t *l2p);
int32 rtl865x_refleshHWL2Table(ether_addr_t * macAddr, uint32 flags,uint16 fid);
extern int32 rtl865x_Lookup_fdb_entry(uint32 fid, ether_addr_t *mac, uint32 flags, uint32 *col_num, rtl865x_tblAsicDrv_l2Param_t *L2buff);
//int32 rtl865x_addFilterDatabaseEntry( uint32 fid, ether_addr_t * mac, uint32 portmask, uint32 type , uint32 isStatic);
int32 rtl865x_addFilterDatabaseEntry( uint32 fid, ether_addr_t * mac, uint32 portmask, uint32 type );
int32 rtl865x_addFilterDatabaseEntryExtension( uint16 fid, rtl865x_filterDbTableEntry_t * L2entry);
int32 rtl865x_getReserveMacAddr(ether_addr_t *macAddr);
int32 rtl865x_delFilterDatabaseEntry(uint16 l2Type, uint16 fid, ether_addr_t * macAddr);
int32 rtl865x_ConvertPortMasktoPortNum(int32 portmask);
int32 rtl865x_delAuthLanFDBEntry(uint16 l2Type,  const unsigned char *addr);
int32 rtl865x_arrangeFdbEntry(const unsigned char *timeout_addr, int32 *port);
int32 rtl865x_lookup_FilterDatabaseEntry(uint16 fid, ether_addr_t * mac, rtl865x_filterDbTableEntry_t *l2_entry);

#ifdef CONFIG_RTL_LINKCHG_PROCESS
int32 rtl865x_LinkChange_Process(void);
#endif

#endif

