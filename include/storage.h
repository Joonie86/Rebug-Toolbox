/**
 * SACD Ripper - http://code.google.com/p/sacd-ripper/
 *
 * Copyright (c) 2010-2011 by respective authors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __SYS_STORAGE_H__
#define __SYS_STORAGE_H__

#include <stdint.h>
#include <string.h>
#include <hvcall.h>
#include <peek_poke.h>
#include <ps3dm_msg.h>
#include <mm.h>
#include "aes_omac.h"

extern float c_firmware;

#ifdef __cplusplus
extern "C" {
#endif

#define BD_DEVICE                           0x0101000000000006ULL
#define HDD_DEVICE                          0x0101000000000007ULL

#define FLASH_DEVICE_NAND                   0x0100000000000001ULL
#define FLASH_DEVICE_NOR                    0x0100000000000004ULL
#define FLASH_FLAGS							0x22ULL

#define VFLASH5_DEV_ID				0x100000500000001ull
#define VFLASH5_SECTOR_SIZE			0x200ull
#define VFLASH5_HEADER_SECTORS		0x2ull

/* The generic packet command opcodes for CD/DVD Logical Units,
 * From Table 57 of the SFF8090 Ver. 3 (Mt. Fuji) draft standard. */
#define GPCMD_GET_CONFIGURATION                0x46
#define GPCMD_GET_EVENT_STATUS_NOTIFICATION    0x4a
#define GPCMD_MODE_SELECT_10                   0x55
#define GPCMD_MODE_SENSE_10                    0x5a
#define GPCMD_READ_CD                          0xbe
#define GPCMD_READ_DVD_STRUCTURE               0xad
#define GPCMD_READ_TRACK_RZONE_INFO            0x52
#define GPCMD_READ_TOC_PMA_ATIP                0x43
#define GPCMD_REPORT_KEY                       0xa4
#define GPCMD_SEND_KEY                         0xa3

#define LV2_STORAGE_SEND_ATAPI_COMMAND         (1)

typedef int sys_io_buffer_t;
typedef int sys_io_block_t;

struct lv2_atapi_cmnd_block
{
    uint8_t  pkt[32];    /* packet command block           */
    uint32_t pktlen;     /* should be 12 for ATAPI 8020    */
    uint32_t blocks;
    uint32_t block_size;
    uint32_t proto;     /* transfer mode                  */
    uint32_t in_out;    /* transfer direction             */
    uint32_t unknown;
} __attribute__((packed));

typedef struct
{
    uint8_t     name[7];
    uint8_t     unknown01;
    uint32_t    unknown02; // random nr?
    uint32_t    zero01;
    uint32_t    unknown03; // 0x28?
    uint32_t    unknown04; // 0xd000e990?
    uint8_t     zero02[16];
    uint64_t    total_sectors;
    uint32_t    sector_size;
    uint32_t    unknown05;
    uint8_t     writable;
    uint8_t     unknown06[3];
    uint32_t    unknown07;
} __attribute__((packed)) device_info_t;

enum lv2_atapi_proto
{
    ATAPI_NON_DATA_PROTO     = 0,
    ATAPI_PIO_DATA_IN_PROTO  = 1,
    ATAPI_PIO_DATA_OUT_PROTO = 2,
    ATAPI_DMA_PROTO          = 3
};

enum lv2_atapi_in_out
{
    ATAPI_DIR_WRITE = 0,   /* memory -> device */
    ATAPI_DIR_READ  = 1    /* device -> memory */
};

static inline void sys_storage_init_atapi_cmnd(
    struct lv2_atapi_cmnd_block *atapi_cmnd
    , uint32_t block_size
    , uint32_t proto
    , uint32_t type)
{
    memset(atapi_cmnd, 0, sizeof(struct lv2_atapi_cmnd_block));
    atapi_cmnd->pktlen     = 12;
    atapi_cmnd->blocks     = 1;
    atapi_cmnd->block_size = block_size;     /* transfer size is block_size * blocks */
    atapi_cmnd->proto      = proto;
    atapi_cmnd->in_out     = type;
}

static inline int sys_storage_send_atapi_command(uint32_t fd, struct lv2_atapi_cmnd_block *atapi_cmnd, uint8_t *buffer)
{
    uint64_t tag;
    system_call_7(616
                , fd
                , LV2_STORAGE_SEND_ATAPI_COMMAND
                , (uint64_t) atapi_cmnd
                , sizeof(struct lv2_atapi_cmnd_block)
                , (uint64_t) buffer
                , atapi_cmnd->block_size
                , (uint64_t) &tag);

    return_to_user_prog(int);
}

static inline int sys_storage_async_configure(uint32_t fd, sys_io_buffer_t io_buffer, sys_event_queue_t equeue_id, int *unknown)
{
    system_call_4(605
                , fd
                , io_buffer
                , equeue_id
                , (uint64_t) unknown);

    return_to_user_prog(int);
}

static inline int sys_storage_get_device_info(uint64_t device, device_info_t *device_info)
{
    system_call_2(609, device, (uint64_t) device_info);
    return_to_user_prog(int);
}

static inline int sys_storage_open(uint64_t id, int *fd)
{
    system_call_4(600, id, 0, (uint64_t) fd, 0);
    return_to_user_prog(int);
}

static inline int sys_storage_close(int fd)
{
    system_call_1(601, fd);
    return_to_user_prog(int);
}

static inline int sys_storage_read(int fd, uint32_t start_sector, uint32_t sectors, uint8_t *bounce_buf, uint32_t *sectors_read)
{
    system_call_7(602, fd, 0, start_sector, sectors, (uint64_t) bounce_buf, (uint64_t) sectors_read, 0);
    return_to_user_prog(int);
}

static inline int sys_storage_read2(int fd, uint32_t start_sector, uint32_t sectors, uint8_t *bounce_buf, uint32_t *sectors_read, uint32_t flags)
{
    system_call_7(602, fd, 0, start_sector, sectors, (uint64_t) bounce_buf, (uint64_t) sectors_read, flags);
    return_to_user_prog(int);
}

static inline int sys_storage_async_read(int fd, uint32_t start_sector, uint32_t sectors, sys_io_block_t bounce_buf, uint64_t user_data)
{
    system_call_7(606, fd, 0, start_sector, sectors, bounce_buf, user_data, 0);
    return_to_user_prog(int);
}

static inline int sys_storage_write(int fd, uint64_t start_sector, uint64_t sectors, uint8_t *buf, uint32_t *sectors_written, uint64_t flags)
{
	system_call_7(603, fd, 0, start_sector, sectors, (uint64_t ) buf, (uint64_t) sectors_written, flags);
	return_to_user_prog(int);
}

static inline int sys_storage_reset_bd(void)
{
    system_call_2(864, 0x5004, 0x29);
    return_to_user_prog(int);
}

static inline int sys_storage_auth_bd(void)
{
    system_call_2(864, 0x5007, 0x43);
    return_to_user_prog(int);
}

static inline int sys_storage_auth_bd_drive(void)
{
    system_call_2(864, 0x5004, 0x46);
    return_to_user_prog(int);
}

static inline int sys_storage_ctrl_bd(int _func)
{
    int func = _func;//0x43;
    system_call_2(864, 0x5007, func);
    return_to_user_prog(int);
}

static inline int sys_storage_setbdemu(void)
{
    int func = 0xA3;
    system_call_2(864, 0x5007, func);
    return_to_user_prog(int);
}

static inline int sys_storage_get_cache_of_flash_ext_flag(uint8_t *flag)
{
    system_call_1(874, (uint64_t) flag);
    return_to_user_prog(int);
}

static inline bool is_nor()
{
	uint8_t vf_flag;
	sys_storage_get_cache_of_flash_ext_flag(&vf_flag);
	return !(vf_flag & 0x1);
}

static inline int get_target_type(uint64_t *type) // 1-CEX, 2-DEX, 3-DECR/RefTool
{
    system_call_3(985, (uint64_t) type, 0, 0);
    return_to_user_prog(int);
}

static inline int lv2_sm_shutdown(uint16_t op, const void *buf, uint64_t size)
{
	system_call_3(379, op, (uint64_t) buf, size);
    return_to_user_prog(int);
}

	struct storage_device_info {
		uint8_t res1[32];
		uint32_t vendor_id;
		uint32_t device_id;
		uint64_t capacity;
		uint32_t sector_size;
		uint32_t media_count;
		uint8_t res2[8];
	};

/*
 * lv2_storage_open
 */
static inline int lv2_storage_open(uint64_t dev_id, uint32_t *dev_handle)
{
	return Lv2Syscall4(600, dev_id, 0, (uint64_t) dev_handle, 0);
}

/*
 * lv2_storage_close
 */
static inline int lv2_storage_close(uint32_t dev_handle)
{
	return Lv2Syscall1(601, dev_handle);
}

/*
 * lv2_storage_read
 */
static inline int lv2_storage_read(uint32_t dev_handle, uint64_t unknown1, uint64_t start_sector, uint64_t sector_count,
	const void *buf, uint32_t *unknown2, uint64_t flags)
{
	return Lv2Syscall7(602, dev_handle, unknown1, start_sector, sector_count,
		(uint64_t ) buf, (uint64_t) unknown2, flags);
}

/*
 * lv2_storage_write
 */
static inline int lv2_storage_write(uint32_t dev_handle, uint64_t unknown1, uint64_t start_sector, uint64_t sector_count,
	const void *buf, uint32_t *unknown2, uint64_t flags)
{
	return Lv2Syscall7(603, dev_handle, unknown1, start_sector, sector_count,
		(uint64_t ) buf, (uint64_t) unknown2, flags);
}

/*
 * lv2_storage_get_device_info
 */
static inline int lv2_storage_get_device_info(uint64_t dev_id, struct storage_device_info *info)
{
	return Lv2Syscall2(609, dev_id, (uint64_t) info);
}


// QA/PROD/RECOVER Toggles

#define AIM_PACKET_ID_GET_DEV_ID				0x19003
#define AIM_PACKET_ID_GET_CONSOLE_ID			0x19005
/*
#define INDI_INFO_MGR_PACKET_ID_GET_DATA_SIZE_BY_INDEX	0x17001
#define INDI_INFO_MGR_PACKET_ID_GET_DATA_BY_INDEX	0x17002
#define EID0_INDEX					0
*/
#define UPDATE_MGR_PACKET_ID_SET_TOKEN			0x600A
#define UPDATE_MGR_PACKET_ID_READ_EPROM			0x600B
#define UPDATE_MGR_PACKET_ID_WRITE_EPROM		0x600C

#define TOKEN_SIZE								80
#define IDPS_SIZE								0x10
#define DEBUG_SUPPORT_FLAG_SIZE					16

#define FSELF_FLAG_OFFSET						0x48C06
#define PRODUCT_MODE_FLAG_OFFSET				0x48C07
#define QA_FLAG_OFFSET							0x48C0A
#define DEVICE_TYPE_FLAG_OFFSET					0x48C13
#define ACTIVE_SPE_FLAG_OFFSET					0x48C30
#define HDD_COPY_MODE_FLAG_OFFSET				0x48C42
#define DEBUG_SUPPORT_FLAG_OFFSET				0x48C50
#define UPDATE_STATUS_FLAG_OFFSET				0x48C60
#define RECOVER_MODE_FLAG_OFFSET				0x48C61
#define QA_TOKEN_OFFSET							0x48D3E

/*
 * lv2_ss_vtrm_mgr_if
 */
static inline int lv2_ss_vtrm_mgr_if(uint32_t packet_id, uint64_t arg1, uint64_t arg2,
	uint64_t arg3, uint64_t arg4)
{
	return Lv2Syscall5(862, packet_id, arg1, arg2, arg3, arg4);
}

/*
 * lv2_ss_update_mgr_if
 */
static inline int lv2_ss_update_mgr_if(uint32_t packet_id, uint64_t arg1, uint64_t arg2,
	uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
	return Lv2Syscall7(863, packet_id, arg1, arg2, arg3, arg4, arg5, arg6);
}

/*
 * lv2_ss_stor_mgr_if
 */
static inline int lv2_ss_stor_mgr_if(uint32_t packet_id, uint64_t arg1)
{
	return Lv2Syscall2(864, packet_id, arg1);
}

/*
 * lv2_ss_secure_rtc_if
 */
static inline int lv2_ss_secure_rtc_if(uint32_t packet_id, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
	return Lv2Syscall4(866, packet_id, arg1, arg2, arg3);
}

/*
 * lv2_ss_aim_if
 */
static inline int lv2_ss_aim_if(uint32_t packet_id, uint64_t arg1)
{
	return Lv2Syscall2(867, packet_id, arg1);
}

/*
 * lv2_ss_indi_info_mgr_if
 */
static inline int lv2_ss_indi_info_mgr_if(uint32_t packet_id, uint64_t arg1, uint64_t arg2,
	uint64_t arg3, uint64_t arg4)
{
	return Lv2Syscall5(868, packet_id, arg1, arg2, arg3, arg4);
}

/*
 * lv2_ss_get_cache_of_flash_ext_flag
 */
static inline int lv2_ss_get_cache_of_flash_ext_flag(uint8_t *flag)
{
	return Lv2Syscall1(874, (uint64_t) flag);
}

/*
 * lv2_get_target_type
 *
 * Types:
 * 	1 - Retail
 * 	2 - Debug
 * 	3 - Reference Tool
 */
static inline int lv2_get_target_type(uint64_t *type)
{
	return Lv2Syscall1(985, (uint64_t) type);
}

// PRODUCT FLAG
u8 read_product_mode_flag()
{	
	u64 value=0;
	if(lv2_ss_update_mgr_if(UPDATE_MGR_PACKET_ID_READ_EPROM,
		PRODUCT_MODE_FLAG_OFFSET, (uint64_t) &value, 0, 0, 0, 0)) return 0;
	return (value==0x00);
}

void set_product_mode_flag()
{
	lv2_ss_update_mgr_if(UPDATE_MGR_PACKET_ID_WRITE_EPROM,
		PRODUCT_MODE_FLAG_OFFSET, 0x00, 0, 0, 0, 0);
}

void reset_product_mode_flag()
{
	lv2_ss_update_mgr_if(UPDATE_MGR_PACKET_ID_WRITE_EPROM,
		PRODUCT_MODE_FLAG_OFFSET, 0xff, 0, 0, 0, 0);
}

void toggle_product_mode_flag()
{	
	if(!read_product_mode_flag())
		set_product_mode_flag();
	else
		reset_product_mode_flag();
}


// RECOVERY FLAG
u8 read_recover_mode_flag()
{	
	u64 value=0;
	if(lv2_ss_update_mgr_if(UPDATE_MGR_PACKET_ID_READ_EPROM,
		RECOVER_MODE_FLAG_OFFSET, (uint64_t) &value, 0, 0, 0, 0)) return 0;
	return (value==0x00);
}

void set_recover_mode_flag()
{
	lv2_ss_update_mgr_if(UPDATE_MGR_PACKET_ID_WRITE_EPROM,
		RECOVER_MODE_FLAG_OFFSET, 0x00, 0, 0, 0, 0);
}

void reset_recover_mode_flag()
{
	lv2_ss_update_mgr_if(UPDATE_MGR_PACKET_ID_WRITE_EPROM,
		RECOVER_MODE_FLAG_OFFSET, 0xff, 0, 0, 0, 0);
}

void toggle_recover_mode_flag()
{	
  if(!read_recover_mode_flag())
		set_recover_mode_flag();
	else
		reset_recover_mode_flag();
}

// QA FLAG
#include "sha1.h"
#include "aes.h"

uint8_t idps[IDPS_SIZE];
uint8_t seed[TOKEN_SIZE];
uint8_t token[TOKEN_SIZE];
AES_KEY aes_ctx;
aes_context aes_ctxt;

void recieve_eid5_idps() 
{
u64 idps0=0;
u64 idps1=0;
	u16 o=0x70;
	u64 start_flash_sector=376; //2f070-> | 30200->303D0 | 70/1D0
	u64 device=FLASH_DEVICE_NOR;

	if(!is_nor())
	{
		start_flash_sector=516;
		device=FLASH_DEVICE_NAND;
	}

		start_flash_sector+=9;
		o=0x1D0;

	u32 readlen=0;
	u64 disc_size=0;
	device_info_t disc_info;
	int rr;
	int dev_id;

	rr=sys_storage_open(device, &dev_id);
	if(!rr) rr=sys_storage_get_device_info(device, &disc_info);
	disc_size = disc_info.sector_size * disc_info.total_sectors;
	u32 buf_size=disc_info.sector_size*1;
	u8* rb = (unsigned char *) memalign(128, buf_size);
	memset(rb, 0, buf_size);

	if(disc_size && !rr)
	{
		rr=sys_storage_read2(dev_id, start_flash_sector, 1, rb, &readlen, FLASH_FLAGS);
		idps0=
			(u64)rb[o + 0] << 56 |
			(u64)rb[o + 1] << 48 |
			(u64)rb[o + 2] << 40 |
			(u64)rb[o + 3] << 32 |
			(u64)rb[o + 4] << 24 |
			(u64)rb[o + 5] << 16 |
			(u64)rb[o + 6] <<  8 |
			(u64)rb[o + 7] <<  0;

		idps1=
			(u64)rb[o + 8] << 56 |
			(u64)rb[o + 9] << 48 |
			(u64)rb[o +10] << 40 |
			(u64)rb[o +11] << 32 |
			(u64)rb[o +12] << 24 |
			(u64)rb[o +13] << 16 |
			(u64)rb[o +14] <<  8 |
			(u64)rb[o +15] <<  0;

		idps[0x0]=rb[o + 0];
		idps[0x1]=rb[o + 1];
		idps[0x2]=rb[o + 2];
		idps[0x3]=rb[o + 3];
		idps[0x4]=rb[o + 4];
		idps[0x5]=rb[o + 5];
		idps[0x6]=rb[o + 6];
		idps[0x7]=rb[o + 7];
		idps[0x8]=rb[o + 8];
		idps[0x9]=rb[o + 9];
		idps[0xa]=rb[o + 10];
		idps[0xb]=rb[o + 11];
		idps[0xc]=rb[o + 12];
		idps[0xd]=rb[o + 13];
		idps[0xe]=rb[o + 14];
		idps[0xf]=rb[o + 15];
		
	}
	rr=sys_storage_close(dev_id);
	free(rb);
}

static uint8_t eid0_key_seed[] = {
	0xAB, 0xCA, 0xAD, 0x17, 0x71, 0xEF, 0xAB, 0xFC,
	0x2B, 0x92, 0x12, 0x76, 0xFA, 0xC2, 0x13, 0x0C, 
	0x37, 0xA6, 0xBE, 0x3F, 0xEF, 0x82, 0xC7, 0x9F, 
	0x3B, 0xA5, 0x73, 0x3F, 0xC3, 0x5A, 0x69, 0x0B, 
	0x08, 0xB3, 0x58, 0xF9, 0x70, 0xFA, 0x16, 0xA3, 
	0xD2, 0xFF, 0xE2, 0x29, 0x9E, 0x84, 0x1E, 0xE4, 
	0xD3, 0xDB, 0x0E, 0x0C, 0x9B, 0xAE, 0xB5, 0x1B, 
	0xC7, 0xDF, 0xF1, 0x04, 0x67, 0x47, 0x2F, 0x85
};

static uint8_t eid0_section_key_seed[] = {
	0x2E, 0xD7, 0xCE, 0x8D, 0x1D, 0x55, 0x45, 0x45,
	0x85, 0xBF, 0x6A, 0x32, 0x81, 0xCD, 0x03, 0xAF
};
	
static uint8_t erk[] = {
	0x34, 0x18, 0x12, 0x37, 0x62, 0x91, 0x37, 0x1c,
	0x8b, 0xc7, 0x56, 0xff, 0xfc, 0x61, 0x15, 0x25,
	0x40, 0x3f, 0x95, 0xa8, 0xef, 0x9d, 0x0c, 0x99,
	0x64, 0x82, 0xee, 0xc2, 0x16, 0xb5, 0x62, 0xed
};

static uint8_t iv_qa[] = {
	0xe8, 0x66, 0x3a, 0x69, 0xcd, 0x1a, 0x5c, 0x45,
	0x4a, 0x76, 0x1e, 0x72, 0x8c, 0x7c, 0x25, 0x4e
};

static uint8_t hmac[] = {
	0xcc, 0x30, 0xc4, 0x22, 0x91, 0x13, 0xdb, 0x25,
	0x73, 0x35, 0x53, 0xaf, 0xd0, 0x6e, 0x87, 0x62,
	0xb3, 0x72, 0x9d, 0x9e, 0xfa, 0xa6, 0xd5, 0xf3,
	0x5a, 0x6f, 0x58, 0xbf, 0x38, 0xff, 0x8b, 0x5f,
	0x58, 0xa2, 0x5b, 0xd9, 0xc9, 0xb5, 0x0b, 0x01,
	0xd1, 0xab, 0x40, 0x28, 0x67, 0x69, 0x68, 0xea,
	0xc7, 0xf8, 0x88, 0x33, 0xb6, 0x62, 0x93, 0x5d,
	0x75, 0x06, 0xa6, 0xb5, 0xe0, 0xf9, 0xd9, 0x7a
};

static uint8_t null_iv[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t eid0_section0_individuals[0x40];
u8 eid0iv[0x10];
uint8_t eid0key[0x20];
uint8_t eid0_section_key[0x10];
uint8_t section0_eid0_enc[0xc0];
u8 section0_eid0_enc_modded[0xc0];
uint8_t section0_eid0_dec[0xc0];
u8* eid_root_key={0};

u8 *_read_buffer(const char *file, u32 *length) {
    FILE *fp;
    u32 size;

    if ((fp = fopen(file, "rb")) == NULL)
        return NULL;

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    u8 *buffer = (u8 *) malloc(sizeof (u8) * size);
    fread(buffer, sizeof (u8), size, fp);

    if (length != NULL)
        *length = size;

    fclose(fp);

    return buffer;
}

/*! Get the number of bits from keysize. */
#define KEY_BITS(ks) (ks * 8)

/*! Isolation root key size. */
#define ISO_ROOT_KEY_SIZE 0x20
/*! Isolation root iv size. */
#define ISO_ROOT_IV_SIZE 0x10

/*! Individuals size. */
#define INDIV_SIZE 0x100
/*! Individuals chunk size. */
#define INDIV_CHUNK_SIZE 0x40

/*! Individuals seed key size. */
#define INDIV_SEED_SIZE 0x40

/*! EID0 keyseed size. */
#define EID0_KEYSEED_SIZE 0x10

void indiv_gen(u8 *seed0, u8 *indiv) {
    u32 i, rounds = 0x100 / 0x40;
    u8 iv[0x10];
	eid_root_key=_read_buffer("/dev_hdd0/game/RBGTLBOX2/USRDIR/eid_root_key", NULL);

    memset(indiv, 0, 0x100);

    //Copy seeds.
    if (seed0 != NULL)
        memcpy(indiv, seed0, 0x40);

    //Generate.
    for (i = 0; i < rounds; i++, indiv += 0x40) {
        //Set key and iv.
		AES_set_encrypt_key(eid_root_key, KEY_BITS(ISO_ROOT_KEY_SIZE), &aes_ctx);
        memcpy(iv, eid_root_key + 0x20, ISO_ROOT_IV_SIZE);
        //Encrypt one chunk.
		AES_cbc_encrypt(iv, indiv, indiv, INDIV_CHUNK_SIZE, &aes_ctx);
    }
}

void cex_dex()
{
sys_timer_usleep(10000);
u8 indiv[0x100];
u8 key[0x10];
indiv_gen(eid0_key_seed, indiv);

    AES_set_encrypt_key(indiv+0x20, 256, &aes_ctx);
	AES_cbc_encrypt(null_iv, eid0_section_key_seed, key, 0x10, &aes_ctx);
	
	
	u32 start_flash_address=0x2f000;
	u64 start_flash_sector=376;
	u64 device=FLASH_DEVICE_NOR;
	if(!is_nor())
	{
		start_flash_address=0x40800;
		start_flash_sector=516;
		device=FLASH_DEVICE_NAND;
	}
	
int dev_id;
u32 readlen=0;
u32 writelen=0;
sys_storage_open(device, &dev_id);
u8 read_buffer[0x200];
sys_storage_read2(dev_id, start_flash_sector, 1, read_buffer, &readlen, FLASH_FLAGS);
recieve_eid5_idps();
int true_dex=0;
int true_dex_dex_idps=0;
if(idps[0x5]==0x82)
{
true_dex=1;
}
else
{
true_dex=0;
}
if((read_buffer[0x75]==0x82) && (!true_dex))
{
read_buffer[0x75]=idps[0x5];
}
else if((read_buffer[0x75]!=0x82) && (!true_dex))
{
read_buffer[0x75]=0x82;
}
else if((read_buffer[0x75]==0x82) && (true_dex))
{
read_buffer[0x75]=0x84;
true_dex_dex_idps=1;
}
else if((read_buffer[0x75]!=0x82) && (true_dex))
{
read_buffer[0x75]=0x82;
true_dex_dex_idps=0;
}
u8 indiv_clone[0x40]; //save my ass from polarssl!
memcpy(indiv_clone, indiv, 0x40);

    aes_setkey_dec(&aes_ctxt, key, 0x80);
    aes_crypt_cbc(&aes_ctxt, AES_DECRYPT, 0xC0, indiv_clone+0x10, read_buffer + 0x90, section0_eid0_dec);
	
//	uint8_t section0_eid0_dec_verify[0xc0];
//	memcpy(section0_eid0_dec_verify, section0_eid0_dec, 0x40);
	u8 omac_verify[0x10];
	aes_omac1(omac_verify, section0_eid0_dec, 0xa8, key, 128);
	u8 digest_default[0x10];
	digest_default[0x00]=section0_eid0_dec[0xa8];
	digest_default[0x01]=section0_eid0_dec[0xa9];
	digest_default[0x02]=section0_eid0_dec[0xaa];
	digest_default[0x03]=section0_eid0_dec[0xab];
	digest_default[0x04]=section0_eid0_dec[0xac];
	digest_default[0x05]=section0_eid0_dec[0xad];
	if(omac_verify[0x00]!=digest_default[0x00] || omac_verify[0x01]!=digest_default[0x01] ||
		omac_verify[0x02]!=digest_default[0x02] || omac_verify[0x03]!=digest_default[0x03] ||
		omac_verify[0x04]!=digest_default[0x04] || omac_verify[0x05]!=digest_default[0x05])
	{
	FILE* omg=fopen("/dev_hdd0/game/RBGTLBOX2/USRDIR/error_cex_dex", "wb");
	fwrite(&omac_verify, 0x10, 1, omg);
	goto done;
	}
	
	if((section0_eid0_dec[0x5]==0x82) && (!true_dex))
	{
	section0_eid0_dec[0x5]=idps[0x5];
	}
	else if((section0_eid0_dec[0x5]!=0x82) &&(!true_dex))
	{
	section0_eid0_dec[0x5]=0x82;
	}
	else if(true_dex && true_dex_dex_idps)
	{
	section0_eid0_dec[0x5]=0x84;
	}
	else if(true_dex && !true_dex_dex_idps)
	{
	section0_eid0_dec[0x5]=0x82;
	}
	u8 digest[0x10];
	aes_omac1(digest, section0_eid0_dec, 0xa8, key, 128);
	
	memcpy(section0_eid0_dec+0xa8, digest, 0x10);
	
//	AES_set_encrypt_key(key, 128, &aes_ctx);
//	AES_cbc_encrypt(indiv+0x10, section0_eid0_dec, section0_eid0_enc_modded, 0xc0, &aes_ctx);

	aes128cbc_enc(key, indiv+0x10, section0_eid0_dec, 0xc0, section0_eid0_enc_modded);


memcpy(read_buffer+0x90, section0_eid0_enc_modded, 0xc0);

sys_storage_write(dev_id, start_flash_sector, 1, read_buffer, &writelen, FLASH_FLAGS);
done:
sys_storage_close(dev_id);

sys_timer_usleep(10000);
return;
}

// READ QA FLAG
u8 read_qa_flag()
{
	u64 value=0;
	lv2_ss_update_mgr_if(UPDATE_MGR_PACKET_ID_READ_EPROM,
		QA_FLAG_OFFSET, (uint64_t) &value, 0, 0, 0, 0);
	return (value==0x00);	
}	

// SET QA FLAG AND TOKEN
void set_qa_flag()
{
/*	int result = lv2_ss_aim_if(AIM_PACKET_ID_GET_DEV_ID, (uint64_t) &idps);
	if(result) return;*/
	int result;
	recieve_eid5_idps();
	if(idps[0x3]!=0x01)
		return;

	memset(seed, 0, TOKEN_SIZE);
	memcpy(seed + 4, idps, IDPS_SIZE);
	seed[3] = 1;

/************************************************************************************************************************/
	
	seed[21] |= 0x7F; /* BDP CONTROL ALL ENABLE DISABLE */
	seed[23] |= 0x7F; /* CONNECT_CONTROL ALL ENABLE DISABLE */
	seed[36] |= 0xF; /* QA_FLAG_RESERVED_FOR_VSH ALL */
	seed[39] |= 0x1; /* QA_FLAG_EXAM_API_ENABLE */
	seed[39] |= 0x2; /* QA_FLAG_QA_MODE_ENABLE */
	seed[43] |= 0x1; /* System Debug */
	seed[43] |= 0x2; /* LV2 System App Debug */
	seed[44] |= 0x9; /* Advanced Token Flag!! */
	seed[47] |= 0x1; /* QA_FLAG_SPECIAL_I */
	seed[47] |= 0x2; /* QA_FLAG_ALLOW_TEST_APP */
	seed[47] |= 0x4; /* QA_FLAG_ALLOW_REMOTE_SPAWN */
	seed[51] |= 0x1; /* QA_FLAG_ALLOW_NON_QA */
	seed[51] |= 0x2; /* QA_FLAG_FORCE_UPDATE */
	
/************************************************************************************************************************/

	hmac_sha1(hmac, sizeof(hmac), seed, 60, seed + 60);

	result = AES_set_encrypt_key(erk, 256, &aes_ctx);
	if(result) return;

	AES_cbc_encrypt(iv_qa, seed, token, TOKEN_SIZE, &aes_ctx);
	
if(c_firmware==3.55f || c_firmware==3.41f)
{

	result = lv2_ss_update_mgr_if(UPDATE_MGR_PACKET_ID_SET_TOKEN,
		(uint64_t) token, TOKEN_SIZE, 0, 0, 0, 0);
	if(result) return;

}
	
else
{
//token start
struct ps3dm_scm_write_eeprom write_eeprom;
u8*p=(u8*)&write_eeprom;
u64 laid, paid, vuart_lpar_addr, muid, nwritten;
int len;
result = lv1_allocate_memory(4096, 0x0C, 0, &vuart_lpar_addr, &muid);
if(result!=0) return;
result = mm_map_lpar_memory_region(vuart_lpar_addr, HV_BASE, HV_SIZE, HV_PAGE_SIZE, 0);
if(result!=0) return;

laid=0x1070000002000001ULL;
paid=0x1070000033000001ULL;
memset(&write_eeprom, 0, sizeof(write_eeprom));
ps3dm_init_header(&write_eeprom.dm_hdr, 1, PS3DM_FID_SCM,
	sizeof(write_eeprom)	-	sizeof(struct ps3dm_header),
	sizeof(write_eeprom)	-	sizeof(struct ps3dm_header));
ps3dm_init_ss_header(&write_eeprom.ss_hdr, PS3DM_PID_SCM_WRITE_EEPROM, laid, paid);
write_eeprom.offset=0x48D3E;
write_eeprom.nwrite=0x50;
write_eeprom.buf_size=0x50;
memset(write_eeprom.buf, 0, sizeof(write_eeprom.buf));
memcpy(write_eeprom.buf, token, 0x50);
len=sizeof(write_eeprom);
for(u16 n =0;n<len;n+=8)
{
static u64 value;
memcpy(&value, &p[n], 8);
lv1_poke((u64) n, value);
__asm__("sync");
value =  lv2_peek(0x8000000000000000ULL);
}
result = lv1_write_virtual_uart(10, vuart_lpar_addr, len, &nwritten);
if(result!=0) return;
nwritten=len;

//token end
}
	
	result = lv2_ss_update_mgr_if(UPDATE_MGR_PACKET_ID_WRITE_EPROM,
		QA_FLAG_OFFSET, 0x00, 0, 0, 0, 0);
}

// RESET QA FLAG AND TOKEN
void reset_qa_flag()
{
/*	int result = lv2_ss_aim_if(AIM_PACKET_ID_GET_DEV_ID, (uint64_t) &idps);
	if(result) return;*/
	int result;
	recieve_eid5_idps();
	if(idps[0x3]!=0x01)
		return;

	memset(seed, 0, TOKEN_SIZE);
	memcpy(seed + 4, idps, IDPS_SIZE);
	seed[3] = 1;

	hmac_sha1(hmac, sizeof(hmac), seed, 60, seed + 60);

	result = AES_set_encrypt_key(erk, 256, &aes_ctx);
	if(result) return;

	AES_cbc_encrypt(iv_qa, seed, token, TOKEN_SIZE, &aes_ctx);
if(c_firmware==3.55f || c_firmware==3.41f)
{
	result = lv2_ss_update_mgr_if(UPDATE_MGR_PACKET_ID_SET_TOKEN,
		(uint64_t) token, TOKEN_SIZE, 0, 0, 0, 0);
	if(result) return;
	
}
else
{
//token start
struct ps3dm_scm_write_eeprom write_eeprom;
u8*p=(u8*)&write_eeprom;
u64 laid, paid, vuart_lpar_addr, muid, nwritten;
int len;
result = lv1_allocate_memory(4096, 0x0C, 0, &vuart_lpar_addr, &muid);
if(result!=0) return;
result = mm_map_lpar_memory_region(vuart_lpar_addr, HV_BASE, HV_SIZE, HV_PAGE_SIZE, 0);
if(result!=0) return;

laid=0x1070000002000001ULL;
paid=0x1070000033000001ULL;
memset(&write_eeprom, 0, sizeof(write_eeprom));
ps3dm_init_header(&write_eeprom.dm_hdr, 1, PS3DM_FID_SCM,
	sizeof(write_eeprom)	-	sizeof(struct ps3dm_header),
	sizeof(write_eeprom)	-	sizeof(struct ps3dm_header));
ps3dm_init_ss_header(&write_eeprom.ss_hdr, PS3DM_PID_SCM_WRITE_EEPROM, laid, paid);
write_eeprom.offset=0x48D3E;
write_eeprom.nwrite=0x50;
write_eeprom.buf_size=0x50;
memset(write_eeprom.buf, 0, sizeof(write_eeprom.buf));
memcpy(write_eeprom.buf, token, 0x50);
len=sizeof(write_eeprom);
for(u16 n =0;n<len;n+=8)
{
static u64 value;
memcpy(&value, &p[n], 8);
lv1_poke((u64) n, value);
__asm__("sync");
value =  lv2_peek(0x8000000000000000ULL);
}
result = lv1_write_virtual_uart(10, vuart_lpar_addr, len, &nwritten);
if(result!=0) return;
nwritten=len;
}
//token end

	lv2_ss_update_mgr_if(UPDATE_MGR_PACKET_ID_WRITE_EPROM,
		QA_FLAG_OFFSET, 0xff, 0, 0, 0, 0);
}

void toggle_qa_flag()
{
	if(!read_qa_flag())
		set_qa_flag();
	else
		reset_qa_flag();
}

#ifdef __cplusplus
};
#endif
#endif /* _SYS_STORAGE_H__ */
