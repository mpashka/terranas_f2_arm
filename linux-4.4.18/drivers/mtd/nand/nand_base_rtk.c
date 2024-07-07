/******************************************************************************
 * $Id: nand_base_rtk.c 317223 2010-06-01 07:38:49Z alexchang2131 $
 * drivers/mtd/nand/nand_base_rtk.c
 * Overview: Realtek MTD NAND Driver 
 * Copyright (c) 2008 Realtek Semiconductor Corp. All Rights Reserved.
 * Modification History:
 *    #000 2008-06-10 Ken-Yu   create file
 *    #001 2008-09-10 Ken-Yu   add BBT and BB management
 *    #002 2008-09-28 Ken-Yu   change r/w from single to multiple pages
 *    #003 2008-10-09 Ken-Yu   support single nand with multiple dies
 *    #004 2008-10-23 Ken-Yu   support multiple nands
 * Copyright (C) 2007-2013 Realtek Semiconductor Corporation
 *******************************************************************************/
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <asm/io.h> 
#include <linux/bitops.h>
#include <linux/parser.h>
#include <linux/semaphore.h>
// Ken-Yu 
#include <mtd/mtd-abi.h>
#include <linux/mtd/rtk_nand_reg.h>
#include <linux/mtd/rtk_nand.h>
//#include <asm/r4kcache.h>
#include <asm/page.h>
#include <linux/jiffies.h>
#include <linux/dma-mapping.h>

//#include <asm/mach-venus/platform.h>
//#include <asm/mach-rtk_dmp/platform.h>

#ifdef CONFIG_MTD_NAND_RTK_HW_SEMAPHORE	

#define HD_SEM_REG	(0xB801A63c)
#define DUMMY_REG	(0xB801A604) 
#define READ_REG(reg )(*(volatile unsigned long *)(reg))
#define WRITE_REG(reg,val)	(*(volatile unsigned long *)(reg) = val)

void rtk_spin_lock()
{	
	while(1) {
		if (READ_REG(HD_SEM_REG) == 0){
		} else {
			break;
		}
	}	
}

void rtk_spin_unlock()
{	
	WRITE_REG(HD_SEM_REG,0);
}

#endif

#ifdef CONFIG_NAND_MULTI_READ_FOR_CARDREADER
	#define NAND_POWEROFF_CARDREADER_WITH_MULTI_READ	(1)//add for fixing card reader share pin bug. alexchang 0831-2010
#else
	#define NAND_POWEROFF_CARDREADER_WITH_MULTI_READ	(0)//add for fixing card reader share pin bug. alexchang 0831-2010
#endif

#ifdef CONFIG_NAND_READ_SKIP_UPDATE_BBT
	#define NAND_READ_SKIP_UPDATE_BBT		(1)//add for fixing card reader share pin bug. alexchang 0901-2010
#else
	#define NAND_READ_SKIP_UPDATE_BBT		(0)//add for fixing card reader share pin bug. alexchang 0901-2010
#endif
//#ifdef CONFIG_NAND_BBT_NEW_MECHANISM
//	#define NAND_BBT_NEW_MECHANISM		(1)//add for fixing card reader share pin bug. alexchang 1026-2010
//	static struct semaphore nf_BBT_sem;
//#else
//	#define NAND_BBT_NEW_MECHANISM		(0)//add for fixing card reader share pin bug. alexchang 1026-2010
//#endif

static DECLARE_RWSEM(rw_sem_resume);
static DECLARE_RWSEM(rw_sem_suspend);
static DECLARE_RWSEM(rw_sem_rd);
static DECLARE_RWSEM(rw_sem_wte);
static DECLARE_RWSEM(rw_sem_erase);
static DECLARE_RWSEM(rw_sem_bbt);
static DECLARE_RWSEM(rw_sem_markbad);
static DECLARE_RWSEM(rw_sem_rd_oob);
static DECLARE_RWSEM(rw_sem_wte_oob);
static DEFINE_SEMAPHORE (sem_rd);
static DEFINE_SEMAPHORE (sem_wte);
static DEFINE_SEMAPHORE (sem_bbt);

#define MAX_NR_LENGTH 1024*1024*2
#define MAX_NW_LENGTH 1024*1024*2
static unsigned char *nwBuffer = NULL;
static unsigned char *nrBuffer = NULL;
//dma_addr_t nrPhys_addr;
//dma_addr_t nwPhys_addr;
static int bEnterSuspend=0;

static unsigned char g_isSysSecure = 0;
extern unsigned char g_isRandomize;
extern unsigned int g_BootcodeSize;
extern  unsigned int g_Factory_param_size;
//extern int g_isCheckEccStatus;
#define Nand_Block_Isbad_Slow_Version 0
#define MTD_SEM_RETRY_COUNT	(0x40)
#define NOTALIGNED(mtd, x) (x & (mtd->writesize-1)) != 0
#define NAND_CP_RW_DISABLE (0xFFFFFFFF)

extern void *map_base;

#define check_end(mtd, addr, len)					\
do {									\
	if (mtd->size == 0) 						\
	{								\
		up_write(&rw_sem_erase);				\
		return -ENODEV;						\
	}								\
	else								\
	if ((addr + len) > mtd->size) {					\
		printk (				\
			"%s: attempt access past end of device\n",	\
			__FUNCTION__);					\
		up_write(&rw_sem_erase);				\
		return -EINVAL;						\
	}								\
} while(0)

/*
#define check_page_align(mtd, addr)					\
do {									\
	if (addr & (mtd->writesize - 1)) {				\
		printk (				\
			"%s: attempt access non-page-aligned data\n",	\
			__FUNCTION__);					\
		printk (				\
			"%s: mtd->writesize = 0x%x\n",			\
			__FUNCTION__,mtd->writesize);			\
		return -EINVAL;						\
	}								\
} while (0)
*/

#define check_block_align(mtd, addr)					\
do {									\
	if (addr & (mtd->erasesize - 1)) {				\
		printk (				\
			"%s: attempt access non-block-aligned data\n",	\
			__FUNCTION__);					\
		up_write(&rw_sem_erase);				\
		return -EINVAL;						\
	}								\
} while (0)

/*
#define check_len_align(mtd,len)					\
do {									\
	if (len & (512 - 1)) {          	 			\
		printk (				\
               	      "%s: attempt access non-512bytes-aligned mem\n",	\
			__FUNCTION__);					\
		return -EINVAL;						\
	}								\
} while (0)
*/

//static DECLARE_MUTEX (mtd_sem);
extern char *rtkNF_parse_token(const char *parsed_string, const char *token);
extern unsigned int rtkNF_getBootcodeSize(void);
extern unsigned int rtkNF_getFactParaSize(void);
unsigned int g_mtd_BootcodeSize = 0;
extern int g_sw_WP_level;
//extern int g_disNFWP;
int g_disNFWP = 0;

extern int is_jupiter_cpu(void);
extern int is_saturn_cpu(void);
extern int is_darwin_cpu(void);
extern int is_macarthur_cpu(void);
extern int is_nike_cpu(void);
extern int is_venus_cpu(void);
extern int is_neptune_cpu(void);
extern int is_mars_cpu(void);
extern void rtk_hexdump( const char * str, unsigned char * pcBuf, unsigned int length );


#if NAND_POWEROFF_CARDREADER_WITH_MULTI_READ
/*
	parse_token can parse a string and extract the value of designated token.
		parsed_string: The string to be parsed.
		token: the name of the token
		return value: If return value is NULL, it means that the token is not found.
			If return value is "non zero", it means that the token is found, and return value will be a string that contains the value of that token.
			If the token doesn't have a value, return value will be a string that contains empty string ("\0").
			If return value is "non zero", "BE SURE" TO free it after you donot need it.

		Exp:
			char *value=parse_token("A1 A2=222 A3=\"333 333\"", "A0");
				=> value=NULL
			char *value=parse_token("A1 A2=222 A3=\"333 333\"", "A1");
				=> value points to a string of "\0"
			char *value=parse_token("A1 A2=222 A3=\"333 333\"", "A2");
				=> value points to a string of "222"
			char *value=parse_token("A1 A2=222 A3=\"333 333\"", "A3");
				=> value points to a string of "333 333"
*/
static char *NF_parse_token(const char *parsed_string, const char *token)
{
	const char *ptr = parsed_string;
	const char *start, *end, *value_start, *value_end;
	char *ret_str;
	
	while(1) {
		value_start = value_end = 0;
		for(;*ptr == ' ' || *ptr == '\t'; ptr++)	;
		if(*ptr == '\0')	break;
		start = ptr;
		for(;*ptr != ' ' && *ptr != '\t' && *ptr != '=' && *ptr != '\0'; ptr++) ;
		end = ptr;
		if(*ptr == '=') {
			ptr++;
			if(*ptr == '"') {
				ptr++;
				value_start = ptr;
				for(; *ptr != '"' && *ptr != '\0'; ptr++)	;
				if(*ptr != '"' || (*(ptr+1) != '\0' && *(ptr+1) != ' ' && *(ptr+1) != '\t')) {
					printk("system_parameters error! Check your parameters.");
					break;
				}
			} else {
				value_start = ptr;
				for(;*ptr != ' ' && *ptr != '\t' && *ptr != '\0' && *ptr != '"'; ptr++) ;
				if(*ptr == '"') {
					printk("system_parameters error! Check your parameters.");
					break;
				}
			}
			value_end = ptr;
		}

		if(!strncmp(token, start, end-start)) {
			if(value_start) {
				ret_str = kmalloc(value_end-value_start+1, GFP_KERNEL);
				strncpy(ret_str, value_start, value_end-value_start);
				ret_str[value_end-value_start] = '\0';
				return ret_str;
			} else {
				ret_str = kmalloc(1, GFP_KERNEL);
				strcpy(ret_str, "");
				return ret_str;
			}
		}
	}

	return (char*)NULL;
}
#endif

#if NAND_POWEROFF_CARDREADER_WITH_MULTI_READ
u32 NF_rtk_power_gpio=0;
u32 NF_rtk_power_bits=0;
void NF_rtkcr_card_power(u8 status)
{
    void __iomem *mmio = (void __iomem *) NF_rtk_power_gpio;
    if(status==0){ //power on
        printk("Card Power on\n");
        writel(readl(mmio) & ~(1<<NF_rtk_power_bits),mmio);
    }else{          //power off
        printk("Card Power off\n");
        writel(readl(mmio) | (1<<NF_rtk_power_bits),mmio);
    }
}
void NF_rtkcr_chk_param(u32 *pparam, u32 len, u8 *ptr)
{
    unsigned int value,i;
    for(i=0;i<len;i++){
        value = ptr[i] - '0';
        if((value >= 0) && (value <=9))
        {
            *pparam+=value<<(4*(len-1-i));
            continue;
        }
        value = ptr[i] - 'a';
        if((value >= 0) && (value <=5))
        {
            value+=10;
            *pparam+=value<<(4*(len-1-i));
            continue;
        }
        value = ptr[i] - 'A';
        if((value >= 0) && (value <=5))
        {
            value+=10;
            *pparam+=value<<(4*(len-1-i));
            continue;
        }
    }
}
int NF_rtkcr_get_gpio(void)
{
    unsigned char *cr_param;
    void __iomem *mmio;
    //cr_param=(char *)NF_parse_token(platform_info.system_parameters,"cr_pw");
    //if(cr_param){
    if (0) {
        NF_rtkcr_chk_param(&NF_rtk_power_gpio,4,cr_param);
        NF_rtkcr_chk_param(&NF_rtk_power_bits,2,cr_param+5);

        mmio = (void __iomem *) (NF_rtk_power_gpio+0xb8010000);
        writel(readl(mmio) | (1<<NF_rtk_power_bits),mmio); //enable GPIO output

        if((NF_rtk_power_gpio & 0xf00) ==0x100){
            NF_rtk_power_gpio+=0xb8010010;
        }else if((NF_rtk_power_gpio & 0xf00) ==0xd00){
            NF_rtk_power_gpio+=0xb8010004;
        }else{
            printk(KERN_ERR "wrong GPIO of card's power.\n");
            return -1;
        }
        printk("power_gpio = 0x%x\n",NF_rtk_power_gpio);
        printk("power_bits = %d\n",NF_rtk_power_bits);
        return 0;
    }
    printk(KERN_ERR "Can't find GPIO of card's power.\n");
    return -1;

}

#endif
/* Realtek supports nand chip types */
/* Micorn */
#define MT29F2G08AAD	0x2CDA8095	//SLC, 256 MB, 1 dies
#define MT29F2G08ABAE	0x2CDA9095  //SLC, 256MB, 1 dies
#define MT29F1G08ABADA  0x2CF18095  //SLC, 128MB, 1 dies
#define MT29F4G08ABADA  0x2CDC9095  //SLC, 512MB, 1 dies
#define MT29F32G08CBACA  0x2C68044A  //MLC, 4GB, 1 dies
#define MT29F64G08CBAAA  0x2C88044B //MLC, 8GB, 1dies
#define MT29F8G08ABABA   0x2C380026  //Micron 1GB   (SLC single die)
//#define MT29F4G08ABAEAH4 0x2CDC90A6  //Micron 4Gb   (SLC single die)
#define MT29F4G08ABAEA	 0x2CDC90A6	// Micron 4Gb  (SLC single die)
#define MT29F64G08CBABA	 0x2C64444B	// Micron 64G	(MLC) 
#define MT29F32G08CBADA	 0x2C44444B	// Micron 32Gb (MLC)

/* STMicorn */
#define NAND01GW3B2B	0x20F1801D	//SLC, 128 MB, 1 dies
#define NAND01GW3B2C	0x20F1001D	//SLC, 128 MB, 1 dies, son of NAND01GW3B2B
#define NAND02GW3B2D	0x20DA1095	//SLC, 256 MB, 1 dies
#define NAND04GW3B2B	0x20DC8095	//SLC, 512 MB, 1 dies
#define NAND04GW3B2D	0x20DC1095	//SLC, 512 MB, 1 dies
#define NAND04GW3C2B	0x20DC14A5	//MLC, 512 MB, 1 dies
#define NAND08GW3C2B	0x20D314A5	//MLC, 1GB, 1 dies

/* Hynix Nand */
#define HY27UF081G2A	0xADF1801D	//SLC, 128 MB, 1 dies
#define HY27UF081G2B	0xADF1801D	//SLC, 128 MB, 1 dies

#define HY27UF082G2A	0xADDA801D	//SLC, 256 MB, 1 dies
#define HY27UF082G2B	0xADDA1095	//SLC, 256 MB, 1 dies
#define HY27UF084G2B	0xADDC1095	//SLC, 512 MB, 1 dies
#define HY27UF084G2M	0xADDC8095	//SLC, 512 MB, 1 dies
	/* HY27UT084G2M speed is slower, we have to decrease T1, T2 and T3 */
#define HY27UT084G2M	0xADDC8425	//MLC, 512 MB, 1 dies, BB check at last page, SLOW nand
#define HY27UT084G2A	0xADDC14A5	//MLC, 512 MB, 1 dies
#define H27U4G8T2B		0xADDC14A5	//MLC, 512 MB, 1 dies
#define HY27UT088G2A	0xADD314A5	//MLC, 1GB, 1 dies, BB check at last page
#define HY27UG088G5M	0xADDC8095	//SLC, 1GB, 2 dies
#define HY27UG088G5B	0xADDC1095	//SLC, 1GB, 2 dies
#define H27U8G8T2B		0xADD314B6	//MLC, 1GB, 1 dies, 4K page
#define H27UAG8T2A		0xADD59425	//MLC, 2GB, 1 dies, 4K page
#define H27UAG8T2B		0xADD5949A	//MLC, 2GB, 1 dies, 8K page
#define H27U2G8F2C		0xADDA9095	//SLC, 256 MB, 1 dies, 2K page
#define H27U4G8F2D      0xADDC9095  //SLC, 512MB, 1dies, 2K page
#define H27U1G8F2B      0xADF1001D	//SLC, 128MB, 1dies, 2K page
#define H27UBG8T2A		0xADD7949A	//MLC, 4GB, 1 dies, 8K page
#define H27UBG8T2B		0xADD794DA	//MLC, 4GB, 1 dies, 8K page
#define H27UBG8T2C		0xADD79491	// Hynix 32Gb	(MLC)
#define H27UCG8T2B		0xADDE94EB	// Hynix 64Gb

/* Samsung Nand */
#define K9F1G08U0E	0xECF10095	//SLC, 128 MB, 1 dies
#define K9F1G08U0D	0xECF10015	//SLC, 128 MB, 1 dies
#define K9F2G08U0B	0xECDA1095	//SLC, 256 MB, 1 dies
#define K9G4G08U0A	0xECDC1425	//MLC, 512 MB, 1 dies, BB check at last page
#define K9G4G08U0B	0xECDC14A5	//MLC, 512 MB, 1 dies, BB check at last page
#define K9F4G08U0B	0xECDC1095	//SLC, 512 MB, 1 dies
#define K9F4G08U0E      0xECDC1095      //SLC, 512 MB, 1 dies
#define K9G8G08U0A	0xECD314A5	//MLC, 1GB, 1 dies, BB check at last page
#define K9G8G08U0M	0xECD31425	//MLC, 1GB, 1 dies, BB check at last page
#define K9K8G08U0A	0xECD35195	//SLC, 1GB, 1 dies
#define K9F8G08U0M	0xECD301A6	//SLC, 1GB, 1 dies, 4K page
#define K9K8G08U0D	0xECD31195  //SLC, 1GB, 1 dies

/* Toshiba */
#define TC58NVG0S3C	0x98F19095	//128 MB, 1 dies
#define TC58NVG0S3E	0x98D19015	//128 MB, 1 dies
#define TC58NVG1S3C	0x98DA9095	//256 MB, 1 dies
#define TC58NVG1S3E	0x98DA9015	//256 MB, 1 dies
#define TC58NVG2S3E	0x98DC9015	//512 MB, 1 dies
#define TC58NVG5D2F	0x98D79432	//MLC, 4GB, 1 dies, 8K page
#define TC58NVG4D2E 0x98D59432  //MLC, 2GB, 1 dies, 8K page
//#define TC58NVG2S0F 0x98DC9026  //SLC,512MB 1 dies,4K page 
#define TC58NVG2S0HTA00 0x98DC9026  //SLC,512MB 1 dies,4K page
#define TC58NVG2S0FTA00	0x98D39026	// Toshiba 4Gb  (SLC single die)
#define TC58NVG5D2H 0x98D79432 
#define TC58BVG0S3H     0x98F18015	//SLC, 1GB, 1 dies
#define TC58BVG1S3H     0x98DA9015	//SLC, 2GB, 1 dies
#define TC58NVG0S3H     0x98F18015	//SLC, 1GB, 1 dies
#define TC58NVG1S3H     0x98DA9015	//SLC, 2GB, 1 dies
#define TC58DVG02D5		0x98f10015  //SLC,128MB 1 dies 
#define TC58TEG5DCJT	0x98D78493	// Toshiba 32Gb (MLC)
#define TC58TEG6DCJT	0x98DE8493	// Toshiba 64Gb (MLC)
#define TC58TEG6DDK     0x98DE9493

/* Macronix/MXIC */
#define MX30LF4G18AC 0xC2DC9095 // 512 MB, 1 dies
#define MX30LF2G18AC 0xC2DA9095 // 256 MB, 1 dies
#define MX30LF1G18AC 0xC2F18095 // 128 MB, 1 dies
#define MX30LF1G08AM 0xC2F1801D // 128 MB, 1 dies
#define MX30LF1208AA 0xC2F0801D // 64MB, 1 dies

#define EN27LN4G08  0xC8DC9095

/* ESMT */
#define F59L1G81MA	0xC8D18095
#define F59L1G81A	0x92F18095
#define F59L2G81A        0xC8DA9095
#define F59L4G81A	0xC8DC9095

/*Winbond*/
#define W29N01GV 0xEFF18095
#define W29N02GV 0xEFDA9095

/*MIRA*/
//#define PSU2GA30AT 0X7F7F7F7F      
#define PSU2GA30BT    0xC8DA9095
#define PSU4GA30AT    0xC8DC9095

/*Spansion*/
#define S34ML01G1	        0x01F1001D
#define S34ML02G1	        0x01DA9095
#define S34ML04G1	        0x01DC9095  
#define S34ML01G2     		0x01F1801D
#define S34ML02G2	        0x01DA9095 
#define S34ML04G2	        0x01DC9095 

/*Zentel*/
#define A5U2GA31BTS	        0xC8DA9095

/*Power Chip*/
#define ASU1GA30HT          0x92F18095


/* RTK Nand Chip ID list */
static int rtd_get_set_nand_info(void);
extern char g_rtk_nandinfo_line[64];
static device_type_t g_nand_device;

static device_type_t nand_device[] = 
{
 {"MT29F2G08AAD", MT29F2G08AAD, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"MT29F2G08ABAE", MT29F2G08ABAE, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x6, 0xff, 0xff,0x01, 0x01, 0x01, 0x00}, 	
 {"MT29F1G08ABADA", MT29F1G08ABADA, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff,0x01, 0x01, 0x01, 0x00},	 
 {"MT29F4G08ABADA", MT29F4G08ABADA, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff,0x01, 0x01, 0x01, 0x00},	  	
 {"MT29F32G08CBACA", MT29F32G08CBACA, 0x100000000, 0x100000000, 4096, 256*4096, 224, 1, 0, 0xa9, 0xff, 0x18,0x01, 0x01, 0x01, 0x18},	
 {"MT29F64G08CBAAA", MT29F64G08CBAAA, 0x200000000, 0x200000000, 8192, 256*8192, 448, 1, 0, 0xff, 0xff, 0x18,0x00, 0x00, 0x00, 0x18},	  	
 {"MT29F8G08ABABA", MT29F8G08ABABA, 0x40000000, 0x40000000, 4096, 128*4096, 224, 1, 0, 0x85, 0xff, 0xff,0x01, 0x01, 0x01, 0x00}, 	 
 //{"MT29F4G08ABAEAH4", MT29F4G08ABAEAH4, 0x20000000, 0x20000000, 4096, 64*4096, 224, 1, 0, 0x54, 0xff, 0xff,0x01, 0x01, 0x01, 0x0c},	  
 {"MT29F4G08ABAEA", MT29F4G08ABAEA, 0x20000000, 0x20000000, 4096, 128*2048, 64, 1, 0, 0x54, 0xff, 0xff,0x01, 0x01, 0x01, 0x0c},	
 //{"MT29F4G08ABAEA", MT29F4G08ABAEA, 0x10000000, 0x10000000, 2048, 128*1024, 64, 1, 0, 0x54, 0xff, 0xff,0x01, 0x01, 0x01, 0x00},
 {"MT29F64G08CBABA", MT29F64G08CBABA, 0x200000000, 0x200000000, 8192, 256*8192, 576, 1, 0, 0xa9, 0xff, 0x28,0x01, 0x01, 0x01, 0x28},
 {"MT29F32G08CBADA", MT29F32G08CBADA, 0x100000000, 0x100000000, 8192, 256*8192, 744, 1, 0, 0xa9, 0xff, 0x28,0x00, 0x00, 0x00, 0x28},

 {"NAND01GW3B2B", NAND01GW3B2B, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"NAND01GW3B2C", NAND01GW3B2C, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"NAND02GW3B2D", NAND02GW3B2D, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x44, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"NAND04GW3B2B", NAND04GW3B2B, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0x20, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"NAND04GW3B2D", NAND04GW3B2D, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"NAND04GW3C2B", NAND04GW3C2B, 0x20000000, 0x20000000, 2048, 128*2048, 64, 1, 1, 0x24, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"NAND08GW3C2B", NAND08GW3C2B, 0x40000000, 0x40000000, 2048, 128*2048, 64, 1, 1, 0x34, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"HY27UF081G2A", HY27UF081G2A, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"HY27UF081G2B", HY27UF081G2B, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"HY27UF082G2A", HY27UF082G2A, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x00, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"HY27UF082G2B", HY27UF082G2B, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x44, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"HY27UF084G2B", HY27UF084G2B, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"HY27UT084G2A", HY27UT084G2A, 0x20000000, 0x20000000, 2048, 128*2048, 64, 1, 1, 0x24, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"H27U4G8T2B", H27U4G8T2B, 0x20000000, 0x20000000, 2048, 128*2048, 64, 1, 1, 0x24, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"HY27UT088G2A", HY27UT088G2A, 0x40000000, 0x40000000, 2048, 128*2048, 64, 1, 1, 0x34, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"HY27UF084G2M", HY27UF084G2M, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"HY27UT084G2M", HY27UT084G2M, 0x20000000, 0x20000000, 2048, 128*2048, 64, 1, 1, 0xff, 0xff, 0xff, 0x04, 0x04, 0x04, 0x00},
 {"HY27UG088G5M", HY27UG088G5M, 0x40000000, 0x20000000, 2048, 64*2048, 64, 2, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"HY27UG088G5B", HY27UG088G5B, 0x40000000, 0x20000000, 2048, 64*2048, 64, 2, 0, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"H27U8G8T2B", H27U8G8T2B, 0x40000000, 0x40000000, 4096, 128*4096, 128, 1, 1, 0x34, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"H27UAG8T2A", H27UAG8T2A, 0x80000000, 0x80000000, 4096, 128*4096, 224, 1, 1, 0x44, 0x41, 0x18, 0x01, 0x01, 0x01, 0x0c},
 {"H27UAG8T2B", H27UAG8T2B, 0x80000000, 0x80000000, 8192, 256*8192, 448, 1, 1, 0x74, 0x42, 0x18, 0x01, 0x01, 0x01, 0x18},
 {"H27U2G8F2C", H27U2G8F2C, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 1, 0x44, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00}, 	 
 {"H27U4G8F2D", H27U4G8F2D, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 1, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},  	
 {"H27U1G8F2B", H27U1G8F2B, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},  	
 {"H27UBG8T2A", H27UBG8T2A, 0x100000000, 0x100000000, 8192, 256*8192, 448, 1, 1, 0x74, 0x42, 0x18, 0x01, 0x01, 0x01, 0x18},
 {"H27UBG8T2B", H27UBG8T2B, 0x100000000, 0x100000000, 8192, 256*8192, 448, 1, 1, 0x74, 0xc3, 0x18, 0x01, 0x01, 0x01, 0x18}, 	 
 {"H27UBG8T2C", H27UBG8T2C, 0x100000000, 0x100000000, 8192, 256*8192, 640, 1, 0, 0x60, 0x44, 0x28, 0x01, 0x01, 0x01, 0x28},
 {"H27UCG8T2B", H27UCG8T2B, 0x200000000, 0x200000000, 16384, 256*16384, 1280, 1, 0, 0x74, 0x44, 0x28, 0x01, 0x01, 0x01, 0x28},
 {"K9F1G08U0E", K9F1G08U0E, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0x41, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"K9F1G08U0D", K9F1G08U0D, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0x40, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"K9F2G08U0B", K9F2G08U0B, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x44, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"K9G4G08U0A", K9G4G08U0A, 0x20000000, 0x20000000, 2048, 128*2048, 64, 1, 1, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"K9G4G08U0B", K9G4G08U0B, 0x20000000, 0x20000000, 2048, 128*2048, 64, 1, 1, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"K9F4G08U0B", K9F4G08U0B, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"K9F4G08U0E", K9F4G08U0E, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0x55, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"K9G8G08U0A", K9G8G08U0A, 0x40000000, 0x40000000, 2048, 128*2048, 64, 1, 1, 0x64, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"K9G8G08U0M", K9G8G08U0M, 0x40000000, 0x40000000, 2048, 128*2048, 64, 1, 1, 0x64, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"K9K8G08U0A", K9K8G08U0A, 0x40000000, 0x40000000, 2048, 64*2048, 64, 1, 0, 0x58, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"K9F8G08U0M", K9F8G08U0M, 0x40000000, 0x40000000, 4096, 64*4096, 128, 1, 0, 0x64, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"K9K8G08U0D", K9K8G08U0D, 0x40000000, 0x40000000, 2048, 64*2048, 64, 1, 0, 0x58, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00}, 	
 {"TC58NVG0S3C", TC58NVG0S3C, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"TC58NVG0S3E", TC58NVG0S3E, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0x76, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"TC58NVG1S3C", TC58NVG1S3C, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"TC58NVG2S3E", TC58NVG2S3E, 0x20000000, 0x20000000, 2048, 64*2048, 64, 1, 0, 0x76, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"TC58NVG5D2F", TC58NVG5D2F, 0x100000000, 0x100000000, 8192, 128*8192, 448, 1, 1, 0x76, 0x55, 0x18, 0x01, 0x01, 0x01, 0x18},
 {"TC58NVG4D2E", TC58NVG4D2E, 0x80000000, 0x80000000, 8192, 128*8192, 448, 1, 1, 0x76, 0x55, 0x18, 0x01, 0x01, 0x01, 0x18},
 //{"TC58NVG2S0F", TC58NVG2S0F, 0x20000000, 0x20000000, 4096, 64*4096, 128, 1, 0, 0x76, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"TC58NVG2S0HTA00", TC58NVG2S0HTA00, 0x20000000, 0x20000000, 4096, 128*2048, 64, 1, 0, 0x76, 0xff, 0xff, 0x01, 0x01, 0x01, 0x0c},
 {"TC58NVG2S0FTA00", TC58NVG2S0FTA00, 0x20000000, 0x20000000, 4096, 64*4096, 128, 1, 0, 0x76, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"TC58TEG5DCJT", TC58TEG5DCJT, 0x100000000, 0x100000000, 16384, 256*16384, 1280, 1, 0, 0x72, 0x57, 0x28, 0x01, 0x01, 0x01, 0x28},
 {"TC58TEG6DCJT", TC58TEG6DCJT, 0x200000000, 0x200000000, 16384, 256*16384, 1280, 1, 0, 0x72, 0x57, 0x28, 0x01, 0x01, 0x01, 0x28},
 {"TC58TEG6DDK", TC58TEG6DDK, 0x200000000, 0x200000000, 16384, 256*16384, 1280, 1, 0, 0x76, 0x50, 0x28, 0x01, 0x01, 0x01, 0x28},

 
 {"TC58NVG5D2H", TC58NVG5D2H, 0x20000000, 0x20000000, 8192, 128*8192, 80, 1, 0, 0x76, 0x56, 0x18, 0x01, 0x01, 0x01, 0x18},
 {"TC58BVG0S3H", TC58BVG0S3H, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xF2, 0x16, 0xff, 0x01, 0x01, 0x01, 0x0c},
 {"TC58BVG1S3H", TC58BVG1S3H, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0xF6, 0x16, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"TC58NVG0S3H", TC58NVG0S3H, 0x8000000, 0x8000000, 2048, 64*2048, 128, 1, 0, 0x72, 0x16, 0xff, 0x01, 0x01, 0x01, 0x0c},
 {"TC58NVG1S3H", TC58NVG1S3H, 0x10000000, 0x10000000, 2048, 64*2048, 128, 1, 0, 0x76, 0x16, 0xff, 0x01, 0x01, 0x01, 0x0c},
 {"TC58NVG1S3E", TC58NVG1S3E, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0x76, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 
 {"TC58DVG02D5", TC58DVG02D5, 0x8000000, 0x8000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},	
 {"W29N01GV", W29N01GV, 0x8000000, 0x8000000, 2048,  64*2048, 64, 1, 0, 0x0, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"W29N02GV", W29N02GV, 0x10000000, 0x10000000, 2048,  64*2048, 64, 1, 0, 0x4, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"MX30LF4G18AC", MX30LF4G18AC, 0x20000000, 0x20000000, 2048,  64*2048, 64, 1, 0, 0x56, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"MX30LF2G18AC", MX30LF2G18AC, 0x10000000, 0x10000000, 2048,  64*2048, 64, 1, 0, 0x07, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"MX30LF1G18AC", MX30LF1G18AC, 0x8000000, 0x8000000, 2048,  64*2048, 64, 1, 0, 0x02, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"MX30LF1G08AM", MX30LF1G08AM, 0x8000000, 0x8000000, 2048,  64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"MX30LF1208AA", MX30LF1208AA, 0x4000000, 0x4000000, 2048,  64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},	 
 {"F59L1G81A", F59L1G81A, 0x8000000, 0x8000000, 2048,  64*2048, 64, 1, 0, 0x40, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},	   
 {"F59L1G81MA", F59L1G81MA, 0x8000000, 0x8000000, 2048,  64*2048, 64, 1, 0, 0x40, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"F59L2G81A", F59L2G81A, 0x10000000, 0x10000000, 2048,  64*2048, 64, 1, 0, 0x44, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 //{"F59L4G81A", F59L4G81A, 0x20000000, 0x20000000, 2048, 64*4096, 64, 1, 1, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"EN27LN4G08", EN27LN4G08, 0x20000000, 0x20000000, 2048,  64*2048, 64, 1, 0, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00}, 	
// {"PSU2GA30AT", PSU2GA30AT, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0xC8, 0xDA, 0xff, 0x00, 0x00, 0x00, 0x00},    
 {"S34ML01G1",S34ML01G1, 0x8000000,  0x8000000,  2048,  64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"S34ML02G1",S34ML02G1, 0x10000000, 0x10000000, 2048,  64*2048, 64, 1, 0, 0x44, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},
 {"S34ML04G1",S34ML04G1, 0x20000000, 0x20000000, 2048,  64*2048, 64, 1, 0, 0x54, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},                
 {"S34ML01G2",S34ML01G2, 0x8000000,  0x8000000,  2048,  64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00},
 {"S34ML02G2",S34ML02G2, 0x10000000, 0x10000000, 2048,  64*2048, 64, 1, 0, 0x46, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00},
 {"S34ML04G2",S34ML04G2, 0x20000000, 0x20000000, 2048,  64*2048, 64, 1, 0, 0x56, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00},
 {"A5U2GA31BTS", A5U2GA31BTS, 0x10000000, 0x10000000, 2048, 64*2048, 64, 1, 0, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00}, 
 {"ASU1GA30HT", ASU1GA30HT, 0x4000000, 0x4000000, 2048,  64*2048, 64, 1, 0, 0x40, 0xff, 0xff, 0x01, 0x01, 0x01, 0x00},	 
     	
 {NULL, }
};

/* NAND low-level MTD interface functions */
static int nand_read (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
static int nand_read_ecc (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, unsigned char *buf, 
			struct mtd_oob_ops *ops);
static int nand_write (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf);
static int nand_write_ecc (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const unsigned char * buf, 
			struct mtd_oob_ops *ops);
static int nand_erase (struct mtd_info *mtd, struct erase_info *instr);
static void nand_sync (struct mtd_info *mtd);
static int nand_suspend (struct mtd_info *mtd);
static void nand_resume (struct mtd_info *mtd);
static int nand_read_oob (struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops);
//static int nand_read_oob_ext (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *oob_buf);
	
static int nand_write_oob (struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops);

/* YAFFS2 */

static int nand_block_isbad (struct mtd_info *mtd, loff_t ofs);
static int nand_block_markbad (struct mtd_info *mtd, loff_t ofs);

/////////////////////////////////////////
int rtk_update_bbt (struct mtd_info *mtd, struct mtd_oob_ops *ops, BB_t *bbt);
int TEST_ERASE_ALL(struct mtd_info *mtd, int page, int bc);



/* Global Variables */
int RBA=0;
static int oob_size, ppb, isLastPage;
static int ppb_shift;
static int page_size = 0;
//CMYu:, 20090415
//extern platform_info_t platform_info;
char *mp_erase_nand;
int mp_erase_flag = 0;
//CMYu:, 20090512
char *mp_time_para;
int mp_time_para_value = 0;
char *nf_clock;
int nf_clock_value = 0;
//CMYu:, 20090720
char *mcp;

//===========================================================================
#if 0
static void NF_CKSEL(char *PartNum, unsigned int value)
{
	REG_WRITE_U32( 0xb800000c,REG_READ_U32(0xb800000c)& (~0x00800000));
	REG_WRITE_U32( 0xb8000034,value);
	REG_WRITE_U32(0xb800000c,REG_READ_U32(0xb800000c)| (0x00800000));
	printk("[%s] %s is set to nf clock: 0x%x\n", __FUNCTION__, PartNum, value);
}
#endif
//------------------------------------------------------------------------------------------------

static unsigned long long rtk_from_to_page(struct mtd_info *mtd, loff_t from)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;

	return (int)(from >> this->page_shift);
}

static unsigned int rtk_find_real_blk(struct mtd_info *mtd, unsigned int blk, int *chipnr_remap)
{
    struct nand_chip *this = (struct nand_chip *) mtd->priv;
    unsigned int i;
    unsigned int real_blk = blk;

    down_write (&rw_sem_bbt);
    for ( i=0; i<RBA; i++){
        if ( this->bbt[i].bad_block != BB_INIT ){
            if ( real_blk == this->bbt[i].bad_block ){
                real_blk = this->bbt[i].remap_block;
                //printk(KERN_DEBUG "%s: Remap ...... [%d] to [%d]\n", __func__, blk, real_blk);
                *chipnr_remap = this->bbt[i].RB_die;
                blk = real_blk;
            }
        }
    }
    if ( this->active_chip != *chipnr_remap ){
        this->active_chip = *chipnr_remap;
        this->select_chip(mtd, *chipnr_remap);
    }
    up_write (&rw_sem_bbt);

    return real_blk;
}

static int  check_BBT(struct mtd_info *mtd, unsigned int blk)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	int i;
	//int BBs=0;
	printk("[%s]..RBA:[%d] blk:[%d]\n", __FUNCTION__, RBA, blk);

	for ( i=0; i<RBA; i++)
	{
		if ( this->bbt[i].bad_block == blk )
		{
			printk("blk 0x%x exist\n",blk);
			return -1;	
		}
	}

	return 0;
}

//------------------------------------------------------------------------------------------------
static int  check_SW_WP(struct mtd_info *mtd)
{
	if(!(mtd->flags & MTD_WRITEABLE))
		return -1;
	//if(!(mtd->flags & MTD_SWP_WRITEABLE))
		//return -1;
	//else
		return 0;
}

//------------------------------------------------------------------------------------------------
static void dump_BBT(struct mtd_info *mtd)
{
	
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	int i;
	int BBs=0;
	printk("[%s] Nand BBT Content\n", __FUNCTION__);
	for ( i=0; i<RBA; i++){
		if ( i==0 && this->bbt[i].BB_die == BB_DIE_INIT && this->bbt[i].bad_block == BB_INIT ){
			printk("Congratulation!! No BBs in this Nand.\n");
			break;
		}
		if ( this->bbt[i].bad_block != BB_INIT ){
			printk(KERN_ERR "[%d] (%d, %u, %d, %u)\n", i, this->bbt[i].BB_die, this->bbt[i].bad_block, 
				this->bbt[i].RB_die, this->bbt[i].remap_block);
			BBs++;
		}
	}
	this->BBs = BBs;
}
#if 0//NAND_BBT_NEW_MECHANISM
int rtk_GetRemapBlk(struct mtd_info *mtd, int chipnr_remap, int block )
{
	struct nand_chip *this = mtd->priv;
	int i=0;
	int retBlk = -1;
	for ( i=0; i<RBA; i++){
	if ( this->bbt[i].bad_block != BB_INIT ){
		if ( this->active_chip == this->bbt[i].BB_die && block == this->bbt[i].bad_block ){
			retBlk = this->bbt[i].remap_block;
			if ( this->bbt[i].BB_die != this->bbt[i].RB_die ){
				this->active_chip = chipnr_remap = this->bbt[i].RB_die;
				this->select_chip(mtd, chipnr_remap);
			}					
		}
	}else
		break;
	}
	return retBlk;
}
//-------------------------------------------------------------------------------------------------
int rtk_BadBlkRemapping(struct mtd_info *mtd, int chipnr, int chipnr_remap, int badblock, int* err_chipnr_remap  )
{
	struct nand_chip *this = mtd->priv;
	int i=0;
	int retBlk = -1;
	while (down_interruptible (&nf_BBT_sem)) {
		printk("%s : Retry\n",__FUNCTION__);
		//return -ERESTARTSYS;
	}
	for( i=0; i<RBA; i++){
		if(this->bbt[i].bad_block==badblock)
		{
			up(&nf_BBT_sem);
			return -1;
		}
	}
	for( i=0; i<RBA; i++){
		if ( this->bbt[i].bad_block == BB_INIT && this->bbt[i].remap_block != RB_INIT){
			if ( chipnr != chipnr_remap)
				this->bbt[i].BB_die = chipnr_remap;
			else
				this->bbt[i].BB_die = chipnr;
			this->bbt[i].bad_block = badblock;
			if(err_chipnr_remap)
				*err_chipnr_remap = this->bbt[i].RB_die;
			retBlk = this->bbt[i].remap_block;
			break;
		}
	}
	up(&nf_BBT_sem);
	if ( retBlk == -1 ){
		printk("[%s] RBA do not have free remap block\n", __FUNCTION__);
		return -1;
	}
	dump_BBT(mtd);
	return retBlk;
}
#endif


#if 0
static void reverse_to_Yaffs2Tags(__u8 *r_oobbuf)
{
	int k;
	int cpBits=0;
	if(page_size==2048)
		cpBits=16;
	else
		cpBits=32;
	for ( k=0; k<cpBits; k++ ){
//	for ( k=0; k<oob_size; k++ ){
		r_oobbuf[k]  = r_oobbuf[1+k];
	}
}
#endif


 static int rtk_block_isbad(struct mtd_info *mtd, u16 chipnr, loff_t ofs)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	unsigned int page, block, page_offset;
	unsigned char block_status_p1;
	unsigned char block_status_p1_org;
#if Nand_Block_Isbad_Slow_Version
	unsigned char block_status_p2;
#endif

	//unsigned char buf[oob_size] __attribute__((__aligned__(4)));

	this->active_chip=chipnr=0;		
	page = ofs >> this->page_shift;
	//printk("[%s] page(%x), ofs(%x), page_shift(%x)\n", __FUNCTION__, page, ofs, this->page_shift);
	block_status_p1_org= this->ops.oobbuf[0];
	page_offset = page & (ppb-1);
	block = page >> ppb_shift;
	printk ("WARNING: Die %d: block=%d is bad, block_status_p1_org=0x%x\n", chipnr, block, block_status_p1_org);
	if ( isLastPage ){
		page = block*ppb + (ppb-1);	
		if ( this->read_oob (mtd, chipnr, page, oob_size, this->ops.oobbuf) ){
			printk ("%s: read_oob page=%d failed\n", __FUNCTION__, page);
			return 1;
		}
		block_status_p1 = this->ops.oobbuf[0];
#if Nand_Block_Isbad_Slow_Version
		page = block*ppb + (ppb-2);	
		if ( this->read_oob (mtd, chipnr, page, oob_size, this->ops.oobbuf) ){
			printk ("%s: read_oob page=%d failed\n", __FUNCTION__, page);
			return 1;
		}
		block_status_p2 = this->ops.oobbuf[0];
		debug_nand("[1]block_status_p1=0x%x, block_status_p2=0x%x\n", block_status_p1, block_status_p2);
#endif		
	}else{	
		if ( this->read_oob (mtd, chipnr, page, oob_size, this->ops.oobbuf) ){
			printk ("%s: read_oob page=%d failed\n", __FUNCTION__, page);
			return 1;
		}
		block_status_p1 = this->ops.oobbuf[0];
#if Nand_Block_Isbad_Slow_Version
		if ( this->read_oob (mtd, chipnr, page+1, oob_size, this->ops.oobbuf) ){
			printk ("%s: read_oob page+1=%d failed\n", __FUNCTION__, page+1);
			return 1;
		}
		block_status_p2 = this->ops.oobbuf[0];
		debug_nand("[2]block_status_p1=0x%x, block_status_p2=0x%x\n", block_status_p1, block_status_p2);
#endif
	}
#if Nand_Block_Isbad_Slow_Version
	if ( (block_status_p1 != 0xff) && (block_status_p2 != 0xff) ){
#else
	if ( block_status_p1 != 0xff){
#endif	
		//printk("[%s] page(0x%x), ofs(%llu), page_shift(%llu)\n", __FUNCTION__, page, ofs, this->page_shift);
		printk ("WARNING: Die %d: block=%d is bad, block_status_p1=0x%x\n", chipnr, block, block_status_p1);
		return -1;
	}
	
	return 0;
}


static int nand_block_isbad (struct mtd_info *mtd, loff_t ofs)
{
	return 0;
}


static int nand_block_markbad (struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	unsigned int page, block, page_offset;
	int i;
	int rc = 0;
	unsigned char buf[oob_size] __attribute__((__aligned__(4)));
	int chipnr, chipnr_remap;
down_write(&rw_sem_markbad);

	if (bEnterSuspend)
	{
		printk(KERN_INFO "[%s] - prevent cmd execute while in suspend stage\n",__func__);
		up_write(&rw_sem_markbad);
		return 0;
	}
	
	page = ofs >> this->page_shift;
	this->active_chip = chipnr = chipnr_remap = (int)(ofs >> this->chip_shift);
	page_offset = page & (ppb-1);
	block = page >> ppb_shift;

	this->active_chip=chipnr=chipnr_remap=0;		
	this->select_chip(mtd, chipnr);
	
	for ( i=0; i<RBA; i++){
		if ( this->bbt[i].bad_block != BB_INIT ){
			if ( chipnr == this->bbt[i].BB_die && block == this->bbt[i].bad_block ){
				block = this->bbt[i].remap_block;
				if ( this->bbt[i].BB_die != this->bbt[i].RB_die ){
					this->active_chip = chipnr_remap = this->bbt[i].RB_die;
					this->select_chip(mtd, chipnr_remap);
				}					
			}
		}else
			break;
	}
	page = block*ppb + page_offset;
				
	buf[0]=0x00;
	rc = this->write_oob (mtd, this->active_chip, page, oob_size, buf);
	if (rc) {
			//DEBUG (MTD_DEBUG_LEVEL0, "%s: write_oob failed\n", __FUNCTION__);
			printk("%s: write_oob failed\n", __FUNCTION__);
			up_write(&rw_sem_markbad);
			return -1;
	}
	up_write(&rw_sem_markbad);
	return 0;
}

//----------------------------------------------------------------------------------------------------
//add by alexchang 0928-2010

//----------------------------------------------------------------------------------------------------
/*
static int nand_read_oob_ext (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, 
			u_char *oob_buf)
{
	struct nand_chip *this = mtd->priv;
	unsigned int page, realpage;
	int oob_len = 0, thislen;
	int rc=0;
	int old_page, page_offset, block;
	int chipnr, chipnr_remap;
	int i;
	
	if (bEnterSuspend)
	{
		printk(KERN_INFO "[%s] - prevent cmd execute while in suspend stage\n",__func__);
		return 0;
	}

	if ((from + len) > mtd->size) {
		printk ("nand_read_oob: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}


	if (NOTALIGNED (mtd, from)) {
		printk (KERN_NOTICE "nand_read_oob: Attempt to read not page aligned data\n");
		return -EINVAL;
	}

	down_write(&rw_sem_rd_oob);

	realpage = (int)(from >> this->page_shift);
	this->active_chip = chipnr = chipnr_remap = (int)(from >> this->chip_shift);
	old_page = page = realpage & this->pagemask;
	page_offset = page & (ppb-1);
	block = page >> ppb_shift;
	this->active_chip=chipnr=chipnr_remap=0;		
	this->select_chip(mtd, chipnr);

	
	if ( retlen ) 
		*retlen = 0;
	thislen = oob_size;

	while (oob_len < len) {
		if (thislen > (len - oob_len)) 
			thislen = (len - oob_len);
		down_write (&rw_sem_bbt);
		for ( i=0; i<RBA; i++){
			if ( this->bbt[i].bad_block != BB_INIT ){
				if ( this->active_chip == this->bbt[i].BB_die && block == this->bbt[i].bad_block ){
					block = this->bbt[i].remap_block;
					if ( this->bbt[i].BB_die != this->bbt[i].RB_die ){
						this->active_chip = chipnr_remap = this->bbt[i].RB_die;
						this->select_chip(mtd, chipnr_remap);
					}					
				}
			}else
				break;
		}
		up_write (&rw_sem_bbt);
		page = block*ppb + page_offset;
	
		rc = this->read_oob (mtd, this->active_chip, page, thislen, &oob_buf[oob_len]);
		if (rc < 0) {
			if (rc == -1){
				printk ("%s: read_oob: Un-correctable HW ECC\n", __FUNCTION__);
				if(check_BBT(mtd,page >> ppb_shift)==0)
				{
				down_write (&rw_sem_bbt);
				for( i=0; i<RBA; i++){
					if ( this->bbt[i].bad_block == BB_INIT && this->bbt[i].remap_block != RB_INIT){
						if ( chipnr != chipnr_remap)	//remap block is bad
							this->bbt[i].BB_die = chipnr_remap;
						else
							this->bbt[i].BB_die = chipnr;
						this->bbt[i].bad_block = page >> ppb_shift;
						break;
					}
				}
				up_write (&rw_sem_bbt);
				dump_BBT(mtd);
				
				if ( rtk_update_bbt (mtd, &this->ops, this->bbt) ){
					printk("[%s] rtk_update_bbt() fails\n", __FUNCTION__);
					up_write(&rw_sem_rd_oob);
					return -1;
					}
				}
				
				this->ops.oobbuf[0] = 0x00;
				if ( isLastPage ){
					this->erase_block (mtd, this->active_chip, block*ppb);
					this->write_oob(mtd, this->active_chip, block*ppb+ppb-1, oob_size, this->ops.oobbuf);
					this->write_oob(mtd, this->active_chip, block*ppb+ppb-2, oob_size, this->ops.oobbuf);
				}else{
					this->erase_block (mtd, this->active_chip, block*ppb);
					this->write_oob(mtd, this->active_chip, block*ppb, oob_size, this->ops.oobbuf);
					this->write_oob(mtd, this->active_chip, block*ppb+1, oob_size, this->ops.oobbuf);
				}
				printk("rtk_read_oob: Un-correctable HW ECC Error at page=%d\n", page);				
			}else{
				printk ("%s: rtk_read_oob: semphore failed\n", __FUNCTION__);
				up_write(&rw_sem_rd_oob);
				return -1;
			}
		}
		
		oob_len += thislen;

		old_page++;
		page_offset = old_page & (ppb-1);
		if ( oob_len<len && !(old_page & this->pagemask)) {
			old_page &= this->pagemask;
			chipnr++;
			this->active_chip = chipnr;
			this->select_chip(mtd, chipnr);
		}
		block = old_page >> ppb_shift;
	}

	if ( retlen ){
		if ( oob_len == len )
			*retlen = oob_len;
		else{
			printk("[%s] error: oob_len %d != len %zu\n", __FUNCTION__, oob_len, len);
			up_write(&rw_sem_rd_oob);
			return -1;
		}	
	}
	up_write(&rw_sem_rd_oob);

	return 0;
}
*/


static int nand_read (struct mtd_info *mtd, loff_t from, size_t len, size_t * retlen, u_char * buf)
{
	int rc=0;
	//unsigned int retryCnt = MTD_SEM_RETRY_COUNT;
	size_t new_len = 0;
	u_char *new_buf = NULL;
	loff_t new_from = from;
	loff_t is_over_page;
	struct nand_chip *this = mtd->priv;

    // Ignore read with length 0
down_write(&rw_sem_rd);
	if(len <= 0){
		*retlen = 0;
		printk("%s:%d read non-positive len=%zu\n", __func__, __LINE__, len);
		up_write(&rw_sem_rd);
		return 0;
	}
	if (bEnterSuspend)
	{
		printk(KERN_INFO "[%s] - prevent cmd execute while in suspend stage\n",__func__);
		up_write(&rw_sem_rd);
		return 0;
	}
#ifdef CONFIG_MTD_NAND_RTK_HW_SEMAPHORE		
	rtk_spin_lock();
#endif	
	if (nrBuffer == NULL)
	{
		//nrBuffer = (unsigned char *)dma_alloc_coherent(&mtd->dev, MAX_NR_LENGTH, (dma_addr_t *) (&nrPhys_addr), GFP_KERNEL);
		//printk("%s:%d allocate from dma_alloc_coherent, nrBuffer=0x%.8x, nrPhys_addr=0x%.8x\n", __func__, __LINE__, nrBuffer, nrPhys_addr);
		nrBuffer = kmalloc( MAX_NR_LENGTH, GFP_KERNEL );
	}
	
	if(g_isSysSecure||g_isRandomize)
  {
        if(g_isRandomize)
			//mtd->isCPdisable_R = 0;
		if(g_isSysSecure)
		{
			//if(*retlen == NAND_CP_RW_DISABLE)
				//mtd->isCPdisable_R = 1;
			//else
				//mtd->isCPdisable_R = 0;
		}
//		if(g_isRandomize)
//		{
//			//mtd->isScramble= 1;
//			mtd->isCPdisable_R = 0;
//		}
	}
	//printk("[%s]scramble 0x%x\n",__FUNCTION__,mtd->isScramble);
	
	if (NOTALIGNED(mtd, from)) {
		//new_from = (from >> this->page_shift) << this->page_shift;
		new_from = from & this->page_idx_mask;
	}

	// Fix over-page calculation.
	is_over_page = (from+len-1) & this->page_idx_mask;
	if ((is_over_page-new_from) > ((len-1)&this->page_idx_mask))
		is_over_page = 1;
	else
		is_over_page = 0;

	if(1) {
	//if (NOTALIGNED(mtd, len)) {
		// Fix length calculation.
		new_len = (((len-1)>>this->page_shift)+1+is_over_page)<<this->page_shift;

		//printk("%s:%d cpu%d  old_len=%d, new_len=%d, from=%lld, new_from=%lld, is_over_page=%lld, page_idx_mask=%lld\n", 
		//	__func__, __LINE__, raw_smp_processor_id(), len, new_len, from, new_from, is_over_page, this->page_idx_mask);

		if (new_len <= MAX_NR_LENGTH)
		{
			new_buf = nrBuffer;
		}
		else
		{
			new_buf = kmalloc( new_len, GFP_KERNEL );
			printk("%s:%d cpu%d  len=%zu\n", __func__, __LINE__, raw_smp_processor_id(), new_len);
		}

		if (new_buf) {
			//rc = nand_read_ecc (mtd, from, new_len, retlen, new_buf, NULL, NULL);
			rc = nand_read_ecc (mtd, new_from, new_len, retlen, new_buf, NULL);
					
			if (rc >= 0) {
				*retlen = len;
				//memcpy(buf, new_buf, len);
				//printk("%s:%d from: %x, new_from: %x, len: %x\n", __func__, __LINE__,from,new_from,len);
				memcpy(buf, new_buf+(from-new_from), len);
			}
			else {
				*retlen = 0;
			}
			//printk("[%s] from:%llu, len(0x%x), new_from:%llu, retlen(0x%x)\n", __FUNCTION__, from, len, new_from, *retlen);
			if (new_len > MAX_NR_LENGTH)
			{
				kfree(new_buf);
			}
		}
	}
	else {
		new_buf = kmalloc( len, GFP_KERNEL );
		
		//printk("[%s] mtd->writesize =%u\n", __FUNCTION__, mtd->writesize);
		//rc = nand_read_ecc (mtd, from, len, retlen, buf, NULL, NULL);

		if (new_buf) {
			//rc = nand_read_ecc (mtd, from, len, retlen, new_buf, NULL, NULL);
			rc = nand_read_ecc (mtd, new_from, len, retlen, new_buf, NULL);
			if (rc >= 0) {
				//memcpy(buf, new_buf, len);
				memcpy(buf, new_buf+(from-new_from), len);
			}
			else {
				*retlen = 0;
			}
			//printk("[%s] from:%llu, len(0x%x), new_from:%llu, retlen(0x%x)\n", __FUNCTION__, from, len, new_from, *retlen);
			kfree(new_buf);
		}
	}
	
	//rtk_hexdump("data_buf : ", buf, *retlen);
	
	if(g_isSysSecure||g_isRandomize)
        {
		//printk("[%s] done \n",__FUNCTION__);
		//mtd->isScramble= 0;
	}

#ifdef CONFIG_MTD_NAND_RTK_HW_SEMAPHORE				
	rtk_spin_unlock();
#endif	
up_write(&rw_sem_rd);

	return rc;
}                          

static int nand_read_oob (struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int rc = 0;
	int chipnr_remap;
	u_char *data_buf = NULL;
	unsigned long long real_page = 0;
	unsigned long long page = 0;
	unsigned long long page_offset = 0;
	unsigned int block = 0;
	unsigned int real_block = 0;
	loff_t new_from = from;

	down_write(&rw_sem_rd);

	data_buf = (ops->datbuf)?(ops->datbuf):this->ops.datbuf;

	from += ops->ooboffs;
	ops->oobretlen = 0;

	page = rtk_from_to_page(mtd, new_from);
	page_offset = page & (ppb-1);
	block = page >> ppb_shift;

	real_block = rtk_find_real_blk(mtd, block, &chipnr_remap);
	real_page = (real_block * ppb) + page_offset;

	rc = this->read_ecc_page(mtd, this->active_chip, real_page, data_buf, ops, CP_NF_NONE);

	ops->oobretlen = oob_size;

	up_write(&rw_sem_rd);

	return rc;
}

static int nand_bad_block_copy (struct mtd_info *mtd, int chipnr_remap, unsigned int real_block, unsigned int page, unsigned char *rbuf, const unsigned char *wbuf)
{
	struct nand_chip *this = mtd->priv;
	BB_t *bbt_p = NULL;
	int i;
	int rc = 0;
	unsigned int max_bitflips = 0;
	struct mtd_oob_ops ops = {0};

	ops.mode = MTD_OPS_PLACE_OOB;
	ops.len = page_size;
	ops.ooblen = oob_size;

	if ( rbuf || wbuf ) {
		ops.datbuf = kmalloc( page_size, GFP_KERNEL );
		ops.oobbuf = kmalloc( oob_size, GFP_KERNEL );
		if ( !ops.datbuf  || !ops.oobbuf ){
			printk("%s: Error, no enough memory for BB copy!\n", __func__);
			return -ENOMEM;
		}
		memset(ops.datbuf, 0xff, page_size);
		memset(ops.oobbuf, 0xff, oob_size);
	}

	//update BBT
	if(check_BBT(mtd,real_block)==0) {
		down_write (&rw_sem_bbt);
		for( i=0; i<RBA; i++){
			if ( this->bbt[i].bad_block == BB_INIT && this->bbt[i].remap_block != RB_INIT) {
				this->bbt[i].BB_die = chipnr_remap;
				this->bbt[i].bad_block = real_block;
				bbt_p = &this->bbt[i];
				break;
			}
		}
		up_write (&rw_sem_bbt);
		dump_BBT(mtd);
		if ( NULL == bbt_p || i >= RBA ) {
			printk("KERN_ERR [%s] RBA do not have free remap block\n", __FUNCTION__);
		return -1;
		}

		if ( rtk_update_bbt (mtd, &this->ops, this->bbt) ) {
			printk("[%s] rtk_update_bbt() fails\n", __FUNCTION__);
			return -1;
		}
	}

	this->select_chip(mtd, chipnr_remap);
	this->erase_block(mtd, chipnr_remap, bbt_p->remap_block*ppb);
	/* Skip block copy on erase failure */
	if ( rbuf || wbuf ) {
		/* Block copy */
		for ( i=0; i<ppb; i++){
			/* Do write on target page */
			if (wbuf && real_block*ppb+i == page) {
				if ( bbt_p->BB_die != bbt_p->RB_die ){
					this->active_chip = bbt_p->RB_die;
					this->select_chip(mtd, bbt_p->RB_die);
				}
				this->write_ecc_page (mtd, this->active_chip, bbt_p->remap_block*ppb+i, wbuf, NULL, 0);
				continue;
			}
			/* Switch to read mode */
			if ( bbt_p->BB_die != bbt_p->RB_die ){
				this->active_chip = bbt_p->BB_die;
				this->select_chip(mtd, bbt_p->BB_die);
			}
			rc = this->read_ecc_page(mtd, this->active_chip, real_block*ppb+i, ops.datbuf, &ops, CP_NF_NONE);
			/* Skip write on clean page */
			if(rc >= 0 && ops.retlen){
				if ( bbt_p->BB_die != bbt_p->RB_die ){
					this->active_chip = bbt_p->RB_die;
					this->select_chip(mtd, bbt_p->RB_die);
				}

				if(this->write_ecc_page (mtd, this->active_chip, bbt_p->remap_block*ppb+i, ops.datbuf, NULL, 0))
					printk(KERN_ERR "Failed to write page 0x%x in block %u on block backup.\n",
						bbt_p->remap_block*ppb+i, bbt_p->remap_block);
				max_bitflips = max_t(unsigned int, max_bitflips, rc);
			}
			else if (rc < 0) {
				printk (KERN_ERR "%s: Un-correctable HW ECC on page 0x%x in block %u\n",
					__FUNCTION__, real_block*ppb+i, real_block);
			}
			/* Copy requested page to the buffer*/
			if (rbuf && real_block*ppb+i == page) memcpy(rbuf, ops.datbuf, page_size);
		} // block loop
	}

	if (ops.datbuf) kfree(ops.datbuf);
	if (ops.oobbuf) kfree(ops.oobbuf);

	if ( bbt_p->BB_die != bbt_p->RB_die ){
		this->active_chip = bbt_p->BB_die;
		this->select_chip(mtd, bbt_p->BB_die);
	}

	this->ops.oobbuf[0] = 0x00;
	this->erase_block (mtd, this->active_chip, real_block*ppb);
	if ( isLastPage ){
		this->write_oob(mtd, this->active_chip, real_block*ppb+ppb-1, oob_size, this->ops.oobbuf);
		this->write_oob(mtd, this->active_chip, real_block*ppb+ppb-2, oob_size, this->ops.oobbuf);
	}else{
		this->write_oob(mtd, this->active_chip, real_block*ppb, oob_size, this->ops.oobbuf);
		this->write_oob(mtd, this->active_chip, real_block*ppb+1, oob_size, this->ops.oobbuf);
	}
	printk("[%s]: Mark HW ECC Error at page=0x%x\n", __func__, page);

	return rc;
}

static int nand_read_ecc (struct mtd_info *mtd, loff_t from, size_t len,
			size_t *retlen, unsigned char *buf, struct mtd_oob_ops *ops)
{
	//printk("[%s] mtd->writesize=0x%x, from=[0x%x] len=[0x%x]\n", __FUNCTION__, mtd->writesize, from, len);
	
	struct nand_chip *this = mtd->priv;
	unsigned long long  page, realpage,page_ppb;
	int data_len;
	int rc = 0;
	unsigned int old_page, page_offset, block, real_block;
	int chipnr, chipnr_remap;
	//int tmp_isCPdisable_R = mtd->isCPdisable_R;
	unsigned int max_bitflips = 0;

	if (bEnterSuspend)
	{
		printk(KERN_INFO "[%s] - prevent cmd execute while in suspend stage\n",__func__);
		return 0;
	}

	if ((from + len) > mtd->size) {
		printk ("nand_read_ecc: Attempt read beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}
	//printk("[%s] NOTALIGNED (mtd, from) 0x%x, NOTALIGNED(mtd, len)0x%x\n",__FUNCTION__,NOTALIGNED (mtd, from),NOTALIGNED(mtd, len));

	if (NOTALIGNED (mtd, from) || NOTALIGNED(mtd, len)) {
		printk (KERN_NOTICE "nand_read_ecc: Attempt to read not page aligned data\n");
		*retlen = 0;
		return -EINVAL;
	}

	realpage = (int)(from >> this->page_shift);
	this->active_chip = chipnr = chipnr_remap = (int)(from >> this->chip_shift);
	old_page = page = realpage & this->pagemask;
	page_offset = page & (ppb-1);
	//block = page/ppb;
	page_ppb = page;
	do_div(page_ppb,ppb);
	block = (unsigned int)page_ppb;

	this->select_chip(mtd, chipnr);
	
	if ( retlen )
		*retlen = 0;
	
	data_len = 0;
	if(ops)
		ops->oobretlen = 0;

	while (data_len < len) {
		real_block = rtk_find_real_blk(mtd, block, &chipnr_remap);
		page = real_block*ppb + page_offset;  
		rc = this->read_ecc_page (mtd, this->active_chip, page, &buf[data_len], ops, CP_NF_NONE);
		if (rc < 0) {
			#if	NAND_POWEROFF_CARDREADER_WITH_MULTI_READ
			printk("[%s]Try again..\n",__FUNCTION__);
			NF_rtkcr_card_power(1);//power off card reader.
			rc = this->read_ecc_page (mtd, this->active_chip, page, &buf[data_len], ops, CP_NF_NONE);
			if(rc<0)
			{
			#if NAND_READ_SKIP_UPDATE_BBT	
				return -1;
			#endif
			}
			#endif
			if (rc == -1){
				printk ("%s: read_ecc_page: Un-correctable HW ECC\n", __FUNCTION__);

				/* Mark BB and do block copy */
				if (nand_bad_block_copy (mtd, chipnr_remap, real_block, page, &buf[data_len], NULL) < 0) {
					printk(KERN_ERR "[%s]Failed to set BB and copy\n", __FUNCTION__);
					return -1;
				}
			}else{
				printk ("%s: read_ecc_page:  semphore failed\n", __FUNCTION__);
				return -1;
			}
		}
		else {
			max_bitflips = max_t(unsigned int, max_bitflips, rc);
			if (rc >= (mtd->bitflip_threshold << 1)){
				/* Mark BB and do block copy */
				printk(KERN_INFO "[%s]Do data copy on %d ECC errors\n",__func__, rc);
				if (nand_bad_block_copy (mtd, chipnr_remap, real_block, page, &buf[data_len], NULL) < 0) {
					printk(KERN_ERR "[%s]Failed to set BB\n", __FUNCTION__);
					return -1;
				}
			}
		}
		data_len += page_size;

		if(ops)//add by alexchang 0524-2010
			ops->oobretlen += oob_size;
		
		old_page++;
		page_offset = old_page & (ppb-1);
		if ( data_len<len && !(old_page & this->pagemask)) {
			old_page &= this->pagemask;
			chipnr++;
			this->active_chip = chipnr;
			this->select_chip(mtd, chipnr);
		}
		block = old_page >> ppb_shift;
	}

	if ( retlen ){
		if ( data_len == len )
			*retlen = data_len;
		else{
				printk("[%s] error: data_len %d != len %zu\n", __FUNCTION__, data_len, len);
				return -1;
		}	
	}
	//printk("[%s]OK\n",__FUNCTION__);

	if(rc < 0)
		return rc;

	return max_bitflips;
}

//panic_nand_write without oobbuf.
static int panic_nand_write (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, 
			const u_char * buf)
{
	return nand_write_ecc (mtd, to, len, retlen, buf, NULL);
}

static int nand_write (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf)
{
	int rc = 0;
	//unsigned int retryCnt = MTD_SEM_RETRY_COUNT;
	u_char *new_buf = NULL;

down_write(&rw_sem_wte);
	if (bEnterSuspend)
	{
		printk(KERN_INFO "[%s] - prevent cmd execute while in suspend stage\n",__func__);
		up_write(&rw_sem_wte);
		return 0;
	}
#ifdef CONFIG_MTD_NAND_RTK_HW_SEMAPHORE		
	rtk_spin_lock();
#endif	
	if (nwBuffer == NULL)
	{
                //nwBuffer = (unsigned char *)dma_alloc_coherent(&mtd->dev, MAX_NW_LENGTH, (dma_addr_t *) (&nwPhys_addr), GFP_DMA | GFP_KERNEL);
                //printk("%s:%d allocate from dma_alloc_coherent, nwBuffer=0x%.8x, nwPhys_addr=0x%.8x\n", __func__, __LINE__, nwBuffer, nwPhys_addr);
		nwBuffer = kmalloc( MAX_NW_LENGTH, GFP_KERNEL );
	}

	if(g_isSysSecure||g_isRandomize)
        {
		if(g_isRandomize)
		{
			//mtd->isScramble = 1;
			//mtd->isCPdisable_W = 0;
		}
		//else
			//mtd->isCPdisable_W = 1;//Add by alexchang for disable yaffs cp read. 0614-2011
		//printk("[%s]Set mtd->isCPdisable_W --> 0 ",__FUNCTION__);
		//printk("[%s]Scrabmle flag 0x%x\n",__FUNCTION__,mtd->isScramble);
	}
	//else
		//mtd->isCPdisable_W = 1;

	//printk("[%s] to:%llu, len:%d\n", __FUNCTION__, to, len);
#if 1
	if (len <= MAX_NW_LENGTH)
	{
		new_buf = nwBuffer;
	}
	else
	{
		new_buf = kmalloc( len, GFP_KERNEL );
		printk("%s:%d cpu%d  len=%zu\n", __func__, __LINE__, raw_smp_processor_id(), len);
	}

	if (new_buf) {
		memcpy(new_buf, buf, len);
		rc = (nand_write_ecc (mtd, to, len, retlen, new_buf, NULL));
		if (len > MAX_NW_LENGTH)
		{
			kfree(new_buf);
		}
	}
#else
	rc = (nand_write_ecc (mtd, to, len, retlen, buf, NULL, NULL));
#endif

	//printk("[%s] to:%llu, len(0x%x), retlen(0x%x)\n", __FUNCTION__, to, len, *retlen);

	if(g_isSysSecure||g_isRandomize)
        {
		//printk("[%s] done\n",__FUNCTION__);
		//mtd->isScramble= 0;
	}

#ifdef CONFIG_MTD_NAND_RTK_HW_SEMAPHORE
	rtk_spin_unlock();
#endif	
up_write(&rw_sem_wte);
	return rc;
}

static int nand_write_oob (struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops)//for 2.6.34 YAFFS-->mtd
{
	struct nand_chip *this = mtd->priv;
	int rc = 0;
	u_char *data_buf = NULL;

	down_write(&rw_sem_wte_oob);

	data_buf = (ops->datbuf)?(ops->datbuf):this->ops.datbuf;

	rc =  nand_write_ecc (mtd, to, ops->len, &ops->retlen, data_buf, ops);
	ops->oobretlen = ops->ooblen;
	ops->retlen = ops->len;

	up_write(&rw_sem_wte_oob);

	return rc;
}

static int nand_write_ecc (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, 
			const u_char * buf, struct mtd_oob_ops *ops)
{
	struct nand_chip *this = mtd->priv;
	unsigned long long page, realpage,page_ppb;
	int data_len;
	int rc;
	unsigned int old_page, page_offset, block, real_block;
	int chipnr, chipnr_remap;

	if (bEnterSuspend)
	{
		printk(KERN_INFO "[%s] - prevent cmd execute while in suspend stage\n",__func__);
		return 0;
	}
	if ((to + len) > mtd->size) {
		printk ("nand_write_ecc: Attempt write beyond end of device\n");
		*retlen = 0;
		return -EINVAL;
	}
	if (NOTALIGNED (mtd, to) || NOTALIGNED(mtd, len)) {
		printk (KERN_NOTICE "nand_write_ecc: Attempt to write not page aligned data mtd size: %x\n", mtd->writesize-1);
		return -EINVAL;
	}
	realpage = (unsigned long long)(to >> this->page_shift);
	this->active_chip = chipnr = chipnr_remap = (int)(to >> this->chip_shift);
	old_page = page = realpage & this->pagemask;
	page_offset = page & (ppb-1);
	//block = page/ppb;
	page_ppb = page;
	do_div(page_ppb,ppb);
	block = (unsigned int)page_ppb;

	if(g_sw_WP_level > 0)
	{
		if(!g_disNFWP)
		{
			if(check_SW_WP(mtd)==-1)
			{
				if(page >= (g_BootcodeSize+g_Factory_param_size)/page_size+ppb) 				
				{
					printk("[%s]Permission denied!!!\n",__FUNCTION__);
					g_disNFWP=0;
					return -1;
				}
			}
		}
	}
	this->select_chip(mtd, chipnr);
	
	if ( retlen )
		*retlen = 0;
	
	data_len = 0;
	if(ops)
		ops->oobretlen = 0;

	while ( data_len < len) {
		real_block = rtk_find_real_blk(mtd, block, &chipnr_remap);
		page = real_block*ppb + page_offset;
		rc = this->write_ecc_page (mtd, this->active_chip, page, &buf[data_len], ops, 0);
		if (rc < 0) {
		//if (0) {
			if ( rc == -1){
				printk ("%s: write_ecc_page:  write blk:%llu, page:%llu failed\n", __FUNCTION__,page >> ppb_shift,page&(ppb-1));

				/* Mark BB and do block copy */
				if (nand_bad_block_copy (mtd, chipnr_remap, real_block, page, NULL, &buf[data_len]) < 0) {
					printk(KERN_ERR "[%s]Failed to set BB and copy\n", __FUNCTION__);
					if(g_sw_WP_level > 0)
					{
						if(g_disNFWP) g_disNFWP=0;
					}

					return -1;
				}
			}else{
				if(g_sw_WP_level > 0)
				{
					if(g_disNFWP)
							g_disNFWP=0;
				}
				return -1;
			}	
		}
		data_len += page_size;
		if(ops)
			ops->oobretlen += oob_size;
		
		old_page++;
		page_offset = old_page & (ppb-1);
		if ( data_len<len && !(old_page & this->pagemask)) {
			old_page &= this->pagemask;
			chipnr++;
			this->active_chip = chipnr;
			this->select_chip(mtd, chipnr);
		}
		block = old_page >> ppb_shift;
	}
	if(g_sw_WP_level > 0)
	{
		if(g_disNFWP)
				g_disNFWP=0;
	}

	if ( retlen ){
		
		if ( data_len == len )
			*retlen = data_len;
		else{
			printk("[%s] error: data_len %d != len %zu\n", __FUNCTION__, data_len, len);
			return -1;
		}	
	}
	return 0;
}


static int nand_erase (struct mtd_info *mtd, struct erase_info *instr)
{
	//printk("[%s] addr=%llu, len=%llu\n", __FUNCTION__, instr->addr, instr->len);
	return nand_erase_nand (mtd, instr, 0);
}


int nand_erase_nand (struct mtd_info *mtd, struct erase_info *instr, int allowbbt)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	u_int64_t addr = instr->addr;
	u_int64_t len = instr->len;
	int page, chipnr;
	int old_page, block, real_block;
	u_int64_t elen = 0;
	int rc = 0;
	int realpage, chipnr_remap;

down_write(&rw_sem_erase);
#ifdef CONFIG_MTD_NAND_RTK_HW_SEMAPHORE
	rtk_spin_lock();
#endif

	if (bEnterSuspend)
	{
		printk(KERN_INFO "[%s] - prevent cmd execute while in suspend stage\n",__func__);
		up_write(&rw_sem_erase);
		return 0;
	}

	check_end (mtd, addr, len);
	check_block_align (mtd, addr);

	 instr->fail_addr = 0xffffffff;

	realpage =  addr >> this->page_shift;
	this->active_chip = chipnr = chipnr_remap = addr >> this->chip_shift;
	old_page = page = realpage & this->pagemask;
	block = page >> ppb_shift;
	if(g_sw_WP_level > 0)
	{
		if(!g_disNFWP)
		{
			if(check_SW_WP(mtd)==-1)
			{
				if(page >= (g_BootcodeSize+g_Factory_param_size)/page_size+ppb) 				
				{
					printk("[%s]Permission denied!!!\n",__FUNCTION__);
					g_disNFWP=0;

					up_write(&rw_sem_erase);

					return -1;
				}
			}
		}
	}
	this->select_chip(mtd, chipnr);
	
	instr->state = MTD_ERASING;
	while (elen < len) {
		real_block = rtk_find_real_blk(mtd, block, &chipnr_remap);
		page = real_block*ppb;
		rc = this->erase_block (mtd, this->active_chip, page);
		if (rc) {
			printk ("%s: block erase failed at page address=%llu\n", __FUNCTION__, addr);
			instr->fail_addr = (page << this->page_shift);	

			/* Mark BB and do block copy */
			if (nand_bad_block_copy (mtd, chipnr_remap, real_block, page, NULL, NULL) < 0) {
				printk(KERN_ERR "[%s]Failed to set BB and copy\n", __FUNCTION__);
				if(g_sw_WP_level > 0)
				{
					if(g_disNFWP) g_disNFWP=0;
				}
				up_write(&rw_sem_erase);
				return -1;
			}
		}

		if ( chipnr != chipnr_remap )
			this->select_chip(mtd, chipnr);
			
		elen += mtd->erasesize;

		old_page += ppb;
		
		if ( elen<len && !(old_page & this->pagemask)) {
			old_page &= this->pagemask;
			chipnr++;
			this->active_chip = chipnr;
			this->select_chip(mtd, chipnr);
		}

		block = old_page >> ppb_shift;
	}
	instr->state = MTD_ERASE_DONE;
	if(g_sw_WP_level > 0)
	{
		if(g_disNFWP)
				g_disNFWP=0;
	}

#ifdef CONFIG_MTD_NAND_RTK_HW_SEMAPHORE
	rtk_spin_unlock();
#endif	
	/* Do call back function */
	if (!rc)
		mtd_erase_callback(instr);

	up_write(&rw_sem_erase);
	return rc;
}


static void nand_sync (struct mtd_info *mtd)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	this->state = FL_READY;
}


static int nand_suspend (struct mtd_info *mtd)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	down_write(&rw_sem_suspend);
	bEnterSuspend = 1;
	//this->suspend(mtd);
	up_write(&rw_sem_suspend);
	return 0;
}


static void nand_resume (struct mtd_info *mtd)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	down_write(&rw_sem_resume);
	//this->resume(mtd);
	bEnterSuspend = 0;
	up_write(&rw_sem_resume);
}


static void nand_select_chip(struct mtd_info *mtd, int chip)
{
	switch(chip) {
		case -1:
			REG_WRITE_U32(REG_PD+map_base,0xff);
			break;			
		case 0:
		case 1:
		case 2:
		case 3:
			REG_WRITE_U32(REG_PD+map_base, ~(1 << chip));
			break;
		default:
			REG_WRITE_U32(REG_PD+map_base, ~(1 << 0));
	}
}


static int scan_last_die_BB(struct mtd_info *mtd)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	__u32 start_page = 0;
	uint64_t addr;
	int block_num = this->block_num;
	int block_size = 1 << this->phys_erase_shift;
	int table_index=0;
	int remap_block[RBA];
	int remap_count = 0;
	int i, j;
	int numchips = this->numchips;
	uint64_t chip_size = this->chipsize;
	int rc = 0;
	__u8 *block_status = NULL;
	
	g_mtd_BootcodeSize = rtkNF_getBootcodeSize();
	if(g_mtd_BootcodeSize == -1)
	{
		if(is_darwin_cpu()||is_saturn_cpu()||is_nike_cpu())
			//g_mtd_BootcodeSize = 12*1024*1024;
			g_mtd_BootcodeSize = 31*1024*1024;
		else
			//g_mtd_BootcodeSize = 16*1024*1024;
			g_mtd_BootcodeSize = 62*1024*1024;
	}
	
	if ( numchips>1 ){
		start_page = 0x00000000;
	}else{
		//start_page = 0x01000000;
		start_page = g_mtd_BootcodeSize;
	}		

	printk("[%s] numchips(%x), start_page(%x), ppb(%x)\n", __FUNCTION__, numchips, start_page, ppb);
	
	this->active_chip = numchips-1;
	this->select_chip(mtd, numchips-1);
	
	block_status = kmalloc( block_num, GFP_KERNEL );	
	if ( !block_status ){
		printk("%s: Error, no enough memory for block_status\n",__FUNCTION__);
		rc = -ENOMEM;
		goto EXIT;
	}
	memset ( (__u32 *)block_status, 0, block_num );

	//printk("addr(%llu), chip_size(%llu), block_size(%d), this->phys_erase_shift(%d)\n", addr, chip_size, block_size, this->phys_erase_shift);
	
	for( addr=start_page; addr<chip_size; addr+=block_size ){
		if ( rtk_block_isbad(mtd, numchips-1, addr) ){
			int bb = addr >> this->phys_erase_shift;
			block_status[bb] = 0xff;
		}
	}

	for ( i=0; i<RBA; i++){
		if ( block_status[(block_num-1)-i] == 0x00){
			remap_block[remap_count] = (block_num-1)-i;
			remap_count++;
		}
	}

	if (remap_count<RBA+1){
		for (j=remap_count+1; j<RBA+1; j++)
			remap_block[j-1] = RB_INIT;
	}
		
	for ( i=0; i<(block_num-RBA); i++){
		if (table_index >= RBA) {
			printk("BB number exceeds RBA. \n");
			break;
		}
		if (block_status[i] == 0xff){
			this->bbt[table_index].bad_block = i;
			this->bbt[table_index].BB_die = numchips-1;
			this->bbt[table_index].remap_block = remap_block[table_index];
			this->bbt[table_index].RB_die = numchips-1;
			table_index++;
		}
	}
	
	for( i=table_index; table_index<RBA; table_index++){
		this->bbt[table_index].bad_block = BB_INIT;
		this->bbt[table_index].BB_die = BB_DIE_INIT;
		this->bbt[table_index].remap_block = remap_block[table_index];
		this->bbt[table_index].RB_die = numchips-1;
	}
	
	//kfree(block_status);

	RTK_FLUSH_CACHE((unsigned long) this->bbt, sizeof(BB_t)*RBA);
	
EXIT:
	//if (rc){
		if (block_status)
			kfree(block_status);	
	//}
				
	return 0;
}


static int scan_other_die_BB(struct mtd_info *mtd)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	__u32 start_page;
	uint64_t addr;
	int block_size = 1 << this->phys_erase_shift;
	int j, k;
	int numchips = this->numchips;
	uint64_t chip_size = this->chipsize;

	g_mtd_BootcodeSize = rtkNF_getBootcodeSize();
	if(g_mtd_BootcodeSize == -1)
	{
		if(is_darwin_cpu()||is_saturn_cpu()||is_nike_cpu())
			g_mtd_BootcodeSize = 12*1024*1024;
		else
			g_mtd_BootcodeSize = 16*1024*1024;
	}
	for( k=0; k<numchips-1; k++ ){
		this->active_chip = k;
		this->select_chip(mtd, k);

		if( k==0 ){
			start_page = g_mtd_BootcodeSize;
		}else{
			start_page = 0x00000000;
		}
		
		for( addr=start_page; addr<chip_size; addr+=block_size ){
			if ( rtk_block_isbad(mtd, k, addr) ){
				for( j=0; j<RBA; j++){
					if ( this->bbt[j].bad_block == BB_INIT && this->bbt[j].remap_block != RB_INIT){
						this->bbt[j].bad_block = addr >> this->phys_erase_shift;
						this->bbt[j].BB_die = k;
						this->bbt[j].RB_die = numchips-1;
						break;
					}
				}
			}
		}
	}
	
	RTK_FLUSH_CACHE((unsigned long) this->bbt, sizeof(BB_t)*RBA);
	
	return 0;
}


static int rtk_create_bbt(struct mtd_info *mtd, int page)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	
	int rc = 0;
	u8 *temp_BBT = 0;
	u8 mem_page_num, page_counter=0;
	//int i;
	
	if(page==ppb)
		printk("[%s] nand driver creates B1 !!\n", __FUNCTION__);
	else if(page==(ppb<<1))
		printk("[%s] nand driver creates B2 !!\n", __FUNCTION__);
	else {
		printk("[%s] abort creating BB on page %x !!\n", __FUNCTION__, page);
		return -1;
	}
	
	if ( scan_last_die_BB(mtd) ){
		printk("[%s] scan_last_die_BB() error !!\n", __FUNCTION__);
		return -1; 
	}
 
	if ( this->numchips >1 ){	
		if ( scan_other_die_BB(mtd) ){
			printk("[%s] scan_last_die() error !!\n", __FUNCTION__);
			return -1;
		}
	}
	
	mem_page_num = (sizeof(BB_t)*RBA + page_size-1 )/page_size;
	temp_BBT = kmalloc( mem_page_num*page_size, GFP_KERNEL );
	if ( !temp_BBT ){
		printk("%s: Error, no enough memory for temp_BBT\n",__FUNCTION__);
		return -ENOMEM;
	}
	memset( temp_BBT, 0xff, mem_page_num*page_size);
			
	this->select_chip(mtd, 0);
	
	if ( this->erase_block(mtd, 0, page) ){
		printk("[%s]erase block %d failure !!\n", __FUNCTION__, page >> ppb_shift);
		rc =  -1;
		goto EXIT;
	}
		
	this->ops.oobbuf[0] = BBT_TAG;	
	memcpy( temp_BBT, this->bbt, sizeof(BB_t)*RBA );
	while( mem_page_num>0 ){
		if ( this->write_ecc_page(mtd, 0, page+page_counter, temp_BBT+page_counter*page_size, 
			&this->ops, 1) ){
				printk("[%s] write BBT B%d page %d failure!!\n", __FUNCTION__, 
					page ==0?0:1, page+page_counter);
				rc =  -1;
				goto EXIT;
		}
		page_counter++;
		mem_page_num--;		
	}

EXIT:
	if (temp_BBT)
		kfree(temp_BBT);
		
	return rc;		
}


int rtk_update_bbt (struct mtd_info *mtd, struct mtd_oob_ops *ops, BB_t *bbt)
{
	int rc = 0;
	struct nand_chip *this = mtd->priv;
	unsigned char active_chip = this->active_chip;
	u8 *temp_BBT = 0;
	__u8 *data_buf = ops->datbuf;
	__u8 *oob_buf = ops->oobbuf;
	
	if (bEnterSuspend)
	{
		printk(KERN_INFO "[%s] - prevent cmd execute while in suspend stage\n",__func__);
		return 0;
	}

	oob_buf[0] = BBT_TAG;

	this->select_chip(mtd, 0);	
		
	if ( sizeof(BB_t)*RBA <= page_size){
		memcpy( data_buf, bbt, sizeof(BB_t)*RBA );
	}else{
		temp_BBT = kmalloc( 2*page_size, GFP_KERNEL );
		if ( !(temp_BBT) ){
			printk("%s: Error, no enough memory for temp_BBT\n",__FUNCTION__);
			return -ENOMEM;
		}
		memset(temp_BBT, 0xff, 2*page_size);
		memcpy(temp_BBT, bbt, sizeof(BB_t)*RBA );
		memcpy(data_buf, temp_BBT, page_size);
	}
		
	if ( this->erase_block(mtd, 0, ppb) ){
		printk("[%s]error: erase block 1 failure\n", __FUNCTION__);
	}

	if ( this->write_ecc_page(mtd, 0, ppb, data_buf, ops, 1) ){
		printk("[%s]update BBT B1 page 0 failure\n", __FUNCTION__);
	}else{
		if ( sizeof(BB_t)*RBA > page_size){
			memset(data_buf, 0xff, page_size);
			memcpy( data_buf, temp_BBT+page_size, sizeof(BB_t)*RBA - page_size );
			if ( this->write_ecc_page(mtd, 0, ppb+1, data_buf, ops, 1) ){
				printk("[%s]update BBT B1 page 1 failure\n", __FUNCTION__);
			}
			memcpy(data_buf, temp_BBT, page_size);//add by alexchang 0906-2010
		}	
	}
	//-----------------------------------------------------------------------
	if ( this->erase_block(mtd, 0, ppb<<1) ){
		printk("[%s]error: erase block 1 failure\n", __FUNCTION__);
		return -1;
	}

	if ( this->write_ecc_page(mtd, 0, ppb<<1, data_buf, ops, 1) ){
		printk("[%s]update BBT B2 failure\n", __FUNCTION__);
		return -1;
	}else{
		if ( sizeof(BB_t)*RBA > page_size){
			memset(data_buf, 0xff, page_size);
			memcpy( data_buf, temp_BBT+page_size, sizeof(BB_t)*RBA - page_size );
			if ( this->write_ecc_page(mtd, 0, 2*ppb+1, data_buf, ops, 1) ){
				printk("[%s]error: erase block 2 failure\n", __FUNCTION__);
				return -1;
			}
		}		
	}
	
	this->select_chip(mtd, active_chip);

	if (temp_BBT)
		kfree(temp_BBT);
			
	return rc;
}


static int rtk_nand_scan_bbt(struct mtd_info *mtd)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int rc = 0;
	__u8 isbbt_b1, isbbt_b2;
	u8 *temp_BBT=0;
	u8 mem_page_num, page_counter=0;
	//int i;

#if 0
	if (0) {
		for (i=0 ; i<this->block_num*ppb; i+=ppb) {
			//rc = this->read_oob(mtd, 0, i, oob_size, this->ops.oobbuf);
			rc = this->erase_block(mtd, 0, i);
			printk("erase_block (%d) rc = 0x%x \n", i, rc);
			//rc = this->read_oob(mtd, 0, i, oob_size, this->ops.oobbuf);
		}
	}
#endif

	RTK_FLUSH_CACHE((unsigned long) this->bbt, sizeof(BB_t)*RBA);
	mem_page_num = (sizeof(BB_t)*RBA + page_size-1 )/page_size;
    
    printk("[%s]:[%s] ppb:[%d] mem_page_num:[%d]\n", __FILE__, __func__, ppb, mem_page_num);

	temp_BBT = kmalloc( mem_page_num*page_size, GFP_KERNEL );
	if ( !temp_BBT ){
		printk("%s: Error, no enough memory for temp_BBT\n",__FUNCTION__);
		return -ENOMEM;
	}
	memset( temp_BBT, 0xff, mem_page_num*page_size);

	rc = this->read_ecc_page(mtd, this->active_chip, ppb, this->ops.datbuf, &this->ops, CP_NF_NONE);
	
	isbbt_b1 = this->ops.oobbuf[0];
	printk("[%s] isbbt_b1:[%x] BBT_TAG:[%x]\n", __FUNCTION__, isbbt_b1, BBT_TAG);
	if ( rc >= 0 ){
		if ( isbbt_b1 == BBT_TAG ){
			printk("[%s] have created bbt B1, just loads it !!\n", __FUNCTION__);
			memcpy( temp_BBT, this->ops.datbuf, page_size );
			page_counter++;
			mem_page_num--;

			while( mem_page_num>0 ){
				if ( this->read_ecc_page(mtd, this->active_chip, ppb+page_counter, this->ops.datbuf, &this->ops, CP_NF_NONE) < 0 ){
					printk("[%s] load bbt B1 error!!\n", __FUNCTION__);
					kfree(temp_BBT);
					return -1;
				}
				memcpy( temp_BBT+page_counter*page_size, this->ops.datbuf, page_size );
				page_counter++;
				mem_page_num--;
			}
			memcpy( this->bbt, temp_BBT, sizeof(BB_t)*RBA );
		}
		else{
			printk("[%s] read BBT B1 tags fails, try to load BBT B2\n", __FUNCTION__);
			rc = this->read_ecc_page(mtd, this->active_chip, ppb<<1, this->ops.datbuf, &this->ops, CP_NF_NONE);
			isbbt_b2 = this->ops.oobbuf[0];	
			if ( rc >= 0 ){
				if ( isbbt_b2 == BBT_TAG ){
					printk("[%s] have created bbt B2, just loads it !!\n", __FUNCTION__);
					memcpy( temp_BBT, this->ops.datbuf, page_size );
					page_counter++;
					mem_page_num--;

					while( mem_page_num>0 ){
						if ( this->read_ecc_page(mtd, this->active_chip, 2*ppb+page_counter, this->ops.datbuf, &this->ops, CP_NF_NONE) < 0 ){
							printk("[%s] load bbt B2 error!!\n", __FUNCTION__);
							kfree(temp_BBT);
							return -1;
						}
						memcpy( temp_BBT+page_counter*page_size, this->ops.datbuf, page_size );
						page_counter++;
						mem_page_num--;
					}
					memcpy( this->bbt, temp_BBT, sizeof(BB_t)*RBA );
				}else{
					printk("[%s] read BBT B2 tags fails, nand driver will creat BBT B1 and B2\n", __FUNCTION__);
					rtk_create_bbt(mtd, ppb);
					rtk_create_bbt(mtd, ppb<<1);
				}
			}else{
				printk("[%s] read BBT B2 with HW ECC fails, nand driver will creat BBT B1\n", __FUNCTION__);
				rtk_create_bbt(mtd, ppb);
			}
		}
	}else{
		printk("[%s] read BBT B1 with HW ECC error, try to load BBT B2\n", __FUNCTION__);
		rc = this->read_ecc_page(mtd, this->active_chip, ppb<<1, this->ops.datbuf, &this->ops, CP_NF_NONE);
		isbbt_b2 = this->ops.oobbuf[0];	
		if ( rc >= 0 ){
			if ( isbbt_b2 == BBT_TAG ){
				printk("[%s] have created bbt B1, just loads it !!\n", __FUNCTION__);
				memcpy( temp_BBT, this->ops.datbuf, page_size );
				page_counter++;
				mem_page_num--;

				while( mem_page_num>0 ){
					if ( this->read_ecc_page(mtd, this->active_chip, 2*ppb+page_counter, this->ops.datbuf, &this->ops, CP_NF_NONE) < 0 ){
						printk("[%s] load bbt B2 error!!\n", __FUNCTION__);
						kfree(temp_BBT);
						return -1;
					}
					memcpy( temp_BBT+page_counter*page_size, this->ops.datbuf, page_size );
					page_counter++;
					mem_page_num--;
				}
				memcpy( this->bbt, temp_BBT, sizeof(BB_t)*RBA );
			}else{
				printk("[%s] read BBT B2 tags fails, nand driver will creat BBT B2\n", __FUNCTION__);
				rtk_create_bbt(mtd, ppb<<1);
			}
		}else{
			printk("[%s] read BBT B1 and B2 with HW ECC fails\n", __FUNCTION__);
			kfree(temp_BBT);
			return -1;
		}
	}

	dump_BBT(mtd);

	if (temp_BBT)
		kfree(temp_BBT);
	
	return rc;
}

static void rtd_set_nand_info(char *item)
{
	char *s = NULL;
	unsigned int temp = 0;
#if 0
	if( (s = strstr(item, "id:")) != NULL ){
	}
#endif
	if( (s = strstr(item, "ds:")) != NULL ){
		kstrtoull(s+3, 10, &g_nand_device.size);
		g_nand_device.chipsize = g_nand_device.size;
	}
	else if( (s = strstr(item, "ps:")) != NULL ){
		kstrtouint(s+3, 10, &g_nand_device.PageSize);
	}
	else if( (s = strstr(item, "bs:")) != NULL ){
		kstrtouint(s+3, 10, &g_nand_device.BlockSize);
	}
	else if( (s = strstr(item, "t1:")) != NULL ){
		kstrtouint(s+3, 10, &temp);
		g_nand_device.T1 = (unsigned char)temp;
	}
	else if( (s = strstr(item, "t2:")) != NULL ){
		kstrtouint(s+3, 10, &temp);
		g_nand_device.T2 = (unsigned char)temp;
	}
	else if( (s = strstr(item, "t3:")) != NULL ){
		kstrtouint(s+3, 10, &temp);
		g_nand_device.T3 = (unsigned char)temp;
	}
	else if( (s = strstr(item, "eb:")) != NULL ){
		kstrtouint(s+3, 10, &temp);
		g_nand_device.eccSelect = (unsigned short)temp;
	}
}

static int rtd_get_set_nand_info()
{
	const char * const delim = ",";
	char *sepstr = g_rtk_nandinfo_line;
	char *substr = NULL;

	if(strlen(g_rtk_nandinfo_line) == 0) {
		printk(KERN_NOTICE "No nand info got from lk!!!!\n");
		return -1;
	}

	memset(&g_nand_device, 0x0, sizeof(device_type_t));

	printk(KERN_DEBUG "g_rtk_nandinfo_line:[%s]\n", sepstr);

	substr = strsep(&sepstr, delim);

	do{
		rtd_set_nand_info(substr);

		substr = strsep(&sepstr, delim);
	}while(substr);

	g_nand_device.OobSize = 64;
	g_nand_device.num_chips = 1;
	g_nand_device.isLastPage = 0;

	printk(KERN_INFO "total flash size:[%llu]\n", g_nand_device.size);
	printk(KERN_INFO "chip size:[%llu]\n", g_nand_device.chipsize);
	printk(KERN_INFO "block size:[%u], page size:[%u], oob size:[%u]\n",
		g_nand_device.BlockSize, g_nand_device.PageSize, g_nand_device.OobSize);
	printk(KERN_INFO "t1:[%u], t2:[%u], t3:[%u]\n",
		g_nand_device.T1, g_nand_device.T2, g_nand_device.T3);
	printk(KERN_INFO "ecc bit:[%u]\n", g_nand_device.eccSelect);

	return 1;
}


int rtk_nand_scan(struct mtd_info *mtd, int maxchips)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	unsigned char id[6];
	unsigned int device_size=0;
	unsigned int i;
	unsigned int nand_type_id ;
	int rtk_lookup_table_flag=0;
	unsigned char maker_code;	
	unsigned char device_code; 
	unsigned char B5th;
	unsigned char B6th;
	unsigned int block_size;
	unsigned int num_chips = 1;
	uint64_t chip_size=0;
	unsigned int num_chips_probe = 1;
	uint64_t result = 0;
	uint64_t div_res = 0;
	unsigned int flag_size, mempage_order;
	unsigned int num_coding_blk = 0;
	
	if ( !this->select_chip )
		this->select_chip = nand_select_chip;
	if ( !this->scan_bbt )
		this->scan_bbt = rtk_nand_scan_bbt;

	this->active_chip = 0;
	this->select_chip(mtd, 0);

	//if(platform_info.secure_boot)
	//	g_isSysSecure = 1;
printk("nand_base_rtk version:1029-2015\n");

	mtd->name = "rtk_nand";

#if NAND_READ_SKIP_UPDATE_BBT
		printk("(V)NAND_READ_SKIP_UPDATE_BBT\n");
#else
		printk("(X)NAND_READ_SKIP_UPDATE_BBT\n");
#endif

	if(rtd_get_set_nand_info() > 0)
	{
		REG_WRITE_U32( REG_TIME_PARA1+map_base, g_nand_device.T1);
		REG_WRITE_U32( REG_TIME_PARA2+map_base, g_nand_device.T2);
		REG_WRITE_U32( REG_TIME_PARA3+map_base, g_nand_device.T3);

		device_size = g_nand_device.size;
		chip_size   = g_nand_device.chipsize;
		page_size   = g_nand_device.PageSize;
		block_size  = g_nand_device.BlockSize;
		oob_size    = g_nand_device.OobSize;
		num_chips   = g_nand_device.num_chips;
		isLastPage  = g_nand_device.isLastPage;

		REG_WRITE_U32( REG_TIME_PARA1+map_base, g_nand_device.T1);
		REG_WRITE_U32( REG_TIME_PARA2+map_base, g_nand_device.T2);
		REG_WRITE_U32( REG_TIME_PARA3+map_base, g_nand_device.T3);

		this->ecc_select = g_nand_device.eccSelect;
	}
	else
	{
	while (1) {
		this->read_id(mtd, id);

		this->maker_code = maker_code = id[0];
		this->device_code = device_code = id[1];
		nand_type_id = maker_code<<24 | device_code<<16 | id[2]<<8 | id[3];
		B5th = id[4];
		B6th = id[5];

printk("READ ID:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",id[0],id[1],id[2],id[3],id[4],id[5]);		

        		i=1;//force one die

		if (i > 1){
			num_chips_probe = i;
			printk(KERN_INFO "NAND Flash Controller detects %d dies\n", num_chips_probe);
		}
	
		for (i = 0; nand_device[i].name; i++){
		
			if ( nand_device[i].id==nand_type_id && 
				((nand_device[i].CycleID5th==0xff)?1:(nand_device[i].CycleID5th==B5th))&&
				((nand_device[i].CycleID6th==0xff)?1:(nand_device[i].CycleID6th==B6th))){
				REG_WRITE_U32( REG_TIME_PARA1+map_base,nand_device[i].T1);
				REG_WRITE_U32( REG_TIME_PARA2+map_base,nand_device[i].T2);
				REG_WRITE_U32( REG_TIME_PARA3+map_base,nand_device[i].T3);
				if ( nand_type_id != HY27UT084G2M ){
					REG_WRITE_U32( REG_MULTI_CHNL_MODE+map_base,0x20);
				}
				if (nand_device[i].size == num_chips_probe * nand_device[i].chipsize){
					if ( num_chips_probe == nand_device[i].num_chips ){
						printk("One %s chip has %d die(s) on board\n", 
							nand_device[i].name, nand_device[i].num_chips);
						//mtd->PartNum = nand_device[i].name;
						device_size = nand_device[i].size;
						chip_size = nand_device[i].chipsize;
						page_size = nand_device[i].PageSize;
						block_size = nand_device[i].BlockSize;
						oob_size = nand_device[i].OobSize;
						num_chips = nand_device[i].num_chips;
						isLastPage = nand_device[i].isLastPage;
						rtk_lookup_table_flag = 1;
						REG_WRITE_U32( REG_TIME_PARA1+map_base,nand_device[i].T1);
						REG_WRITE_U32( REG_TIME_PARA2+map_base,nand_device[i].T2);
						REG_WRITE_U32( REG_TIME_PARA3+map_base,nand_device[i].T3);
						printk("index(%d) nand part=%s, id=0x%x, device_size=%llu, page_size=0x%x, block_size=0x%x, oob_size=0x%x, num_chips=0x%x, isLastPage=0x%x, ecc_sel=0x%x \n",
							i,
							nand_device[i].name,
							nand_device[i].id,
							nand_device[i].size,
							page_size,
							block_size,
							oob_size,
							num_chips,
							isLastPage,
							nand_device[i].eccSelect);
						break;
					}				
				}else{
					if ( !strcmp(nand_device[i].name, "HY27UF084G2M" ) )
						continue;
					else{	
						printk("We have %d the same %s chips on board\n", 
							num_chips_probe/nand_device[i].num_chips, nand_device[i].name);
						//mtd->PartNum = nand_device[i].name;
						device_size = nand_device[i].size;
						chip_size = nand_device[i].chipsize;	
						page_size = nand_device[i].PageSize;
						block_size = nand_device[i].BlockSize;
						oob_size = nand_device[i].OobSize;
						num_chips = nand_device[i].num_chips;
						isLastPage = nand_device[i].isLastPage;
						rtk_lookup_table_flag = 1;
						printk("nand part=%s, id=%x, device_size=%llu, chip_size=%llu, num_chips=%d, isLastPage=%d, eccBits=%d\n", 
							nand_device[i].name, nand_device[i].id, nand_device[i].size, nand_device[i].chipsize, 
							nand_device[i].num_chips, nand_device[i].isLastPage, nand_device[i].eccSelect);
						break;
					}
				}
			}
		}

		//g_isCheckEccStatus = 0;
		if ( !rtk_lookup_table_flag ){
			printk("Warning: Lookup Table do not have this nand flash !!\n");
			printk ("%s: Manufacture ID=0x%02x, Chip ID=0x%02x, "
					"3thID=0x%02x, 4thID=0x%02x, 5thID=0x%02x, 6thID=0x%02x\n",
					mtd->name, id[0], id[1], id[2], id[3], id[4], id[5]);
			return -1;
		}

		this->ecc_select = nand_device[i].eccSelect;

		break;

	} // old way of NAND flash detection
	} // Get NAND info from bootcode

	/* Common attributes setup */
	{
		this->page_shift = __ffs(page_size); 
		this->page_idx_mask = ~((1L << this->page_shift) -1);
		this->phys_erase_shift = __ffs(block_size);
		this->oob_shift = __ffs(oob_size);
		ppb = this->ppb = block_size >> this->page_shift;
		ppb_shift = this->phys_erase_shift - this->page_shift;

		if (chip_size){
			this->block_num = chip_size >> this->phys_erase_shift;
			this->page_num = chip_size >> this->page_shift;
			this->chipsize = chip_size;
			this->device_size = device_size;
			this->chip_shift =  __ffs(this->chipsize );
		}

		this->pagemask = (this->chipsize >> this->page_shift) - 1;

		mtd->oobsize = this->oob_size = oob_size;

		mtd->writesize = page_size;
		mtd->erasesize = block_size;
		mtd->writebufsize = page_size;

		mtd->erasesize_shift = this->phys_erase_shift;
		mtd->writesize_shift = this->page_shift;

		// Default to 6 bit BCH ECC if ecc_select = 0
		if(!this->ecc_select) this->ecc_select = 0x6;
		mtd->ecc_strength = this->ecc_select;
		printk("QQ ecc_select 0x%x\n", this->ecc_select);

		if(this->ecc_select>=0x10){
			/* 1KB coding block */
			num_coding_blk = mtd->writesize >> 10;
		}
		else{
			/* 512B coding block */
			num_coding_blk = mtd->writesize >> 9;
		}
		/* Need to consider the ECC error in each mini page? */
		mtd->bitflip_threshold = max(mtd->ecc_strength, 4u) >> 2;

		mtd->ecc_strength *= num_coding_blk;
	}

	this->select_chip(mtd, 0);
			
	if ( num_chips != num_chips_probe )
		this->numchips = num_chips_probe;
	else
		this->numchips = num_chips;
	div_res = mtd->size = this->numchips * this->chipsize;

	do_div(div_res,block_size);
	result = div_res;
	result*=this->RBA_PERCENT;
	do_div(result,100);
	RBA = this->RBA = result;

	//mtd->size = mtd->size - RBA*block_size;

	//RBA = this->RBA = (mtd->size/block_size) * this->RBA_PERCENT/100;
	//if(mtd->u32RBApercentage)
		//mtd->u32RBApercentage = this->RBA_PERCENT;
	//printk("[%s],mtd->u32RBApercentage %d%\n",__FUNCTION__,mtd->u32RBApercentage);
	this->bbt = kmalloc( sizeof(BB_t)*RBA, GFP_KERNEL );

	if ( !this->bbt ){
		printk("%s: Error, no enough memory for BBT\n",__FUNCTION__);
		return -1;
	}
	memset(this->bbt, 0,  sizeof(BB_t)*RBA); 

	this->ops.datbuf = kmalloc( page_size, GFP_KERNEL );
	if ( !this->ops.datbuf ){
		printk("%s: Error, no enough memory for g_databuf\n",__FUNCTION__);
		return -ENOMEM;
	}
	memset(this->ops.datbuf, 0xff, page_size);

	this->ops.oobbuf = kmalloc( oob_size, GFP_KERNEL );
	if ( !this->ops.oobbuf ){
		printk("%s: Error, no enough memory for g_oobbuf\n",__FUNCTION__);
		return -ENOMEM;
	}
	memset(this->ops.oobbuf, 0xff, oob_size);

	flag_size =  (this->numchips * this->page_num) >> 3;
	mempage_order = get_order(flag_size);
	
//	if(mempage_order<13)
	{
		this->erase_page_flag = (void *)__get_free_pages(GFP_KERNEL, mempage_order);	
		if ( !this->erase_page_flag ){
			printk("%s: Error, no enough memory for erase_page_flag\n",__FUNCTION__);
			return -ENOMEM;
		}
		memset ( (__u32 *)this->erase_page_flag, 0, flag_size);
	}
	mtd->type			= MTD_NANDFLASH;
	mtd->flags			= MTD_CAP_NANDFLASH;
	//mtd->ecctype		= MTD_ECC_NONE;
	mtd->_erase			= nand_erase;
	mtd->_point			= NULL;
	mtd->_unpoint		= NULL;
	mtd->_read			= nand_read;
	mtd->_write			= nand_write;
	//mtd->read_ecc		= nand_read_ecc;
	//mtd->write_ecc		= nand_write_ecc;
	mtd->_read_oob		= nand_read_oob;
	mtd->_write_oob	    = nand_write_oob;
	//mtd->readv		= NULL;
	//mtd->writev		= NULL;
	//mtd->readv_ecc	= NULL;
	//mtd->writev_ecc	= NULL; 
	mtd->_sync			= nand_sync;
	mtd->_lock			= NULL;
	mtd->_unlock		= NULL;
	mtd->_suspend		= nand_suspend;
	mtd->_resume		= nand_resume;
	mtd->owner			= THIS_MODULE;

	mtd->_block_isbad	= nand_block_isbad;
	mtd->_block_markbad	= nand_block_markbad;
	mtd->_panic_write   = panic_nand_write;
	/* Ken: 20090210 */
	//mtd->reload_bbt = rtk_nand_scan_bbt;
	
	mtd->owner = THIS_MODULE;

	//return 0;//ignore bbt scan
	return this->scan_bbt(mtd);
}


int TEST_ERASE_ALL(struct mtd_info *mtd, int page, int bc)
{
	struct nand_chip *this = (struct nand_chip *) mtd->priv;
	int i;
	int chip_block_num = this->block_num;
	int start_block = page >> ppb_shift;
	int block_in_die; 
	int rc = 0;
	int chipnr =0, block;
	
	if (bEnterSuspend)
	{
		printk(KERN_INFO "[%s] - prevent cmd execute while in suspend stage\n",__func__);
		return 0;
	}

	if ( page & (ppb-1) ){
		page = (page >> ppb_shift)*ppb;
	}

	for ( i=0; i<bc; i++){
		block_in_die = start_block + i;
		chipnr = block_in_die/chip_block_num;

		this->active_chip=chipnr=0;		
		block = block_in_die%chip_block_num;
		this->select_chip(mtd, block_in_die/chip_block_num);
		rc = this->erase_block(mtd, chipnr, block*ppb);
		if ( rc<0 ){
			this->ops.oobbuf[0] = 0x00;
			if ( isLastPage ){
				this->write_oob(mtd, chipnr, block*ppb+ppb-1, oob_size, this->ops.oobbuf);
				this->write_oob(mtd, chipnr, block*ppb+ppb-2, oob_size, this->ops.oobbuf);
			}else{
				this->write_oob(mtd, chipnr, block*ppb, oob_size, this->ops.oobbuf);
				this->write_oob(mtd, chipnr, block*ppb+1, oob_size, this->ops.oobbuf);
			}
		}
	}

	return 0;
}

