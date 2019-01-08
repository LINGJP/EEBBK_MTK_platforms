/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: yuyz <yuyz@rock-chips.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <errno.h>
#include <signal.h>
#include <linux/watchdog.h>

#include <time.h>

#include "camera_ui.h"
#include "camera_ui_def.h"
#include "ueventmonitor/ueventmonitor.h"
#include "ueventmonitor/usb_sd_ctrl.h"
#include "ueventmonitor/keyboard.h"

#include "network_manage.h"
#include "parameter.h"
#ifdef RK_DEF_SDCARD
#include "fs_manage/fs_manage.h"
#include "fs_manage/fs_storage.h"
#include "fs_manage/fs_sdcard.h"
#endif

#include "common.h"
#include "videoplay.h"
#include <dpp/adas.h>

#include "audioplay.h"
#include <assert.h>

#include <pthread.h>

#include "ui_resolution.h"

#include "watermark.h"
#include "video_api.h"
#include "example/user.h"
#include "videoplay.h"
#include "gsensor.h"
#include "power/thermal.h"
#include "collision/collision.h"
#include "parking_monitor/parking_monitor.h"

#include "fwk_protocol/rk_protocol.h"
#include "fwk_protocol/rk_fwk.h"
#include "fwk_protocol/rk_protocol_gui.h"
#include "fwk_protocol/rk_protocol_device_fault.h"
#include "power_manage.h"
#include "ntp_sync.h"
#include "api/leds_api.h"
#include "storage/storage.h"

#ifdef DEF_RK_ENABLE_UI
#include "ui.h"
#endif

#ifdef RTSP_SERVER
#include "rtsp/rtspserver.h"
#endif
#include <libfwk_msg/fwk_gl_api.h>
#include <libfwk_msg/fwk_gl_def.h>
#include <libfwk_msg/fwk_gl_msg.h>
#include "show.h"
#include <curl/curl.h>
#include "gyrosensor.h"


//#define USE_WATCHDOG
static int hMainWnd = 0;
timer_t g_timer;
#define MODE_RECORDING 0
#define MODE_PHOTO 1

#define BATTERY_CUSHION 3
#define USE_KEY_STOP_USB_DIS_SHUTDOWN

static OdtOutput g_odt_output;
static short screenoff_time;

//battery stable
static int last_battery = 1;

struct bitmap watermark_bmap[7];

static pthread_mutex_t set_mutex;
static int odtFlag = 0;

static bool takephoto = false;

static bool usewatchdog = false;
static int fd_wtd;

//----------------------
static int battery = 0;

static char sdcard = 0;
static int cap;
static int SetMode = MODE_RECORDING;
static int SetCameraMP = 0;

//-------------------------
static bool video_rec_state = 0;
static int sec;
struct FlagForUI FlagForUI_ca;

// test code start
static void* keypress_sound = NULL;
static void* capture_sound = NULL;
// test code end
static struct ui_frame frontcamera[4] = {0}, backcamera[4] = {0};

/*SHUTDOWN PARAMS*/
static int gshutdown = 0;

#define PARKING_ENABLE			1
#define PARKING_RECORD_COUNT	90
#define PARKING_SUSPEND			1
#define PARKING_SHUTDOWN		2

static int gparking_gs_active = 0;
static int gparking_count = 0;
static char gparking = 0;

static long int collision_rec_time = 0;
//
void (*wififormat_callback)(void) = NULL;

static void initrec(HWND hWnd);
static void deinitrec(HWND hWnd);
void exitvideopreview(void);
int startrec(HWND hWnd);
int stoprec(HWND hWnd);

static int shutdown_deinit(HWND hWnd);
static int shutdown_usb_disconnect(HWND hWnd);

#define VOICE_KEY	8
#define LONG_PRESS_RECOVERY_METHOD 1
#if LONG_PRESS_RECOVERY_METHOD
#define RECOVERY_KEY 1
#if LONG_PRESS_RECOVERY_METHOD == 1

#define RECOVERY_PRESS_TIME (6*1.e6)
struct key_long_press_status {
  struct timeval last_press;
  int key_value;
} key_status;

#define COLOR_RED_R 0x1F
#define COLOR_RED_G 0x11
#define COLOR_RED_B 0x11
#define COLOR_GREEN_R 0x11
#define COLOR_GREEN_G 0x3F
#define COLOR_GREEN_B 0x11
#define COLOR_KEY_R 0x0
#define COLOR_KEY_G 0x0
#define COLOR_KEY_B 0x1
static struct win *g_ui_win;

#ifdef ENABLE_RS_FACE
static void *g_ui_clear_buffer = NULL;
#endif

void fb_clear_ui(void)
{
  struct win * ui_win;
  unsigned short rgb565_data;
  unsigned short *ui_buff;
  int i;

  if (!g_ui_win)
    g_ui_win = rk_fb_getuiwin();

  if (!g_ui_win)
    return;

  ui_win = g_ui_win;
  ui_buff = (unsigned short *)ui_win->buffer;

  rgb565_data = (COLOR_KEY_R & 0x1f) << 11 | ((COLOR_KEY_G & 0x3f) << 5) | (COLOR_KEY_B & 0x1f);
#ifdef ENABLE_RS_FACE  
  memcpy((void *)ui_buff, g_ui_clear_buffer, ui_win->width * ui_win->height*2);
#else
  for (i = 0; i < ui_win->width * ui_win->height; i ++) {
    ui_buff[i] = rgb565_data;
  }
#endif
}

static double tvdiff( struct timeval* pre, struct timeval *pos)
{
  double a;
  unsigned int b;

  a = pos->tv_sec - pre->tv_sec;
  if(pos->tv_usec > pre->tv_usec) {
    b = pos->tv_usec - pre->tv_usec;
    return a*1.e6 + b;
  } else {
    b = pre->tv_usec - pos->tv_usec;
    return a*1.e6 - b;
  }
}
#endif
#endif

#if defined(NTP_SYNC_REQUEST) /* ntp */
ntp_server ntp_servers = {
  .nof = 5,
  .host = {
    "1.cn.pool.ntp.org",
    "tw.pool.ntp.org",
    "cn.ntp.org.cn",
    "210.72.145.44",
    "cn.pool.ntp.org"
  },
};

int ntp_synced_callback(void* arg, int msg, int param)
{
  HWND hWnd = (HWND)arg;

  DBGMSG(CI,("%s enter\n", __func__));
  if (wififormat_callback != NULL) {
    wififormat_callback();
    FlagForUI_ca.formatflag = 0;
    video_post_msg(hWnd, MSG_SDCHANGE, 0, 1);
    return;
  }
  if (msg == NTP_SYNC_TO) {
    video_post_msg(hWnd, MSG_NTP_SYNCED, 0, EVENT_NTP_SYNC_FAIL);
  } else if(msg == NTP_SYNC_OK) {
    {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      DBGMSG(CM, ("gettimeofday %ld %ld\n", tv.tv_sec, tv.tv_usec));
    }
    usleep(500*1000);/*wait for video frame refresh*/
    video_post_msg(hWnd, MSG_NTP_SYNCED, 0, EVENT_NTP_SYNC_FINISH);
    if (SetMode == MODE_RECORDING) {
      DBGMSG(CI,("SetMode == MODE_RECORDING\n"));
      if (sdcard == 1) {
        DBGMSG(CI,("sdcard == 1\n"));
        if (parameter_get_video_de())
          start_motion_detection(hWnd);
        else if (parameter_get_video_autorec())
          startrec(hWnd);
      }
    }
  }
  DBGMSG(CI,("%s exit\n", __func__));
}

int ntp_wait_for_sync(ntp_callback callback, void *puser, unsigned int to)
{
  int r = 0;

  DBGMSG(CI, ("ntp_wait_for_sync: enter!!!\n"));
  if (ntp_synced()) {
    DBGMSG(CI, ("synced ok,exit\n"));
    return r;
  }
  ntp_ac_pre();
  if(ntp_enable()) {
    if (ntp_synced()) {
      DBGMSG(CI, ("synced again ok,exit\n"));
      ntp_ac_post();
      return r;
    }
    if(!ntp_is_register_callback(callback)) {
      r = ntp_register_callback(callback, (void*)puser, to);
      if(r) { /*call back register ok, then stop record if is recording*/
        stoprec((HWND)puser);
      }
    }
    r = 1;
  }
  ntp_ac_post();
  DBGMSG(CI, ("ntp_wait_for_sync: %d!!!\n", r));
  return r;
}
#endif


void (*storage_filesize_event_call)(int cmd, void *msg0, void *msg1);

int storage_setting_reg_event_callback(void (*call)(
                                                    int cmd,
                                                    void *msg0,
                                                    void *msg1))
{
#ifdef RK_DEF_SDCARD
	storage_filesize_event_call = call;
#else
	storage_filesize_event_call = NULL;
#endif
	return 0;
}

void storage_setting_event_callback(int cmd, void *msg0, void *msg1)
{
#ifdef RK_DEF_SDCARD
	if (storage_filesize_event_call)
		return (*storage_filesize_event_call)(cmd, msg0, msg1);
#endif
}


enum {
    UI_FORMAT_BGRA_8888 = 0x1000,
    UI_FORMAT_RGB_565,
};

void parking_event_callback(int cmd, void *msg0, void *msg1)
{
	switch (cmd) {
	case CMD_PARKING:
		gparking_gs_active = 1;
		break;
	default:
		break;
	}
}
void hdmi_event_callback(int cmd, void *msg0, void *msg1)
{
  //printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);
  video_post_msg(hMainWnd, MSG_HDMI, (WPARAM)msg0, (LPARAM)msg1);
}

void mouse_event_callback(int cmd, void *msg0, void *msg1)
{
    //printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);
    video_post_msg(hMainWnd, MSG_MOUSE, (WPARAM)msg0, (LPARAM)msg1);
}

void camere_event_callback(int cmd, void *msg0, void *msg1)
{
  //printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);
  video_post_msg(hMainWnd, MSG_CAMERA, (WPARAM)msg0, (LPARAM)msg1);
}

int collision_event_callback(int cmd, void *msg0, void *msg1)	//gsensor hui diao xinxi lyq
{
	switch (cmd) {
	case CMD_COLLISION:
#ifdef CACHE_ENCODEDATA_IN_MEM
		video_post_msg(hMainWnd, MSG_COLLISION,
				(WPARAM)msg0, (LPARAM)msg1);

		return COLLISION_CACHE_DURATION;
#else
		#ifdef RK_DEF_SDCARD
		if((SetMode == MODE_RECORDING) && video_rec_state) {
			video_record_blocknotify(BLOCK_PREV_NUM, BLOCK_LATER_NUM);
			if (collision_rec_time >= COLLISION_CACHE_DURATION) {
				video_record_savefile();
				collision_rec_time = 0;
				return COLLISION_CACHE_DURATION;
			}

			return (COLLISION_CACHE_DURATION - collision_rec_time);
		}
		#endif
#endif
		break;
	default:
		break;
	}

	return 0;
}

void user_event_callback(int cmd, void *msg0, void *msg1)
{
  //printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);
  switch (cmd)
  {
  #ifdef RK_DEF_SDCARD
    case CMD_USER_RECORD_RATE_CHANGE:
      video_post_msg(hMainWnd, MSG_RECORD_RATE_CHANGE, (WPARAM)msg0, (LPARAM)msg1);
	    break;
  #endif
	  case CMD_USER_MDPROCESSOR:
      video_post_msg(hMainWnd, MSG_ATTACH_USER_MDPROCESSOR, (WPARAM)msg0, (LPARAM)msg1);
      break;
	  case CMD_USER_MUXER:
      video_post_msg(hMainWnd, MSG_ATTACH_USER_MUXER, (WPARAM)msg0, (LPARAM)msg1);
      break;
  }
}
void show_odt_result(void ){
	       parameter_get_video_odt();
		   /* clear background */
		   fb_clear_ui();
		   /* Draw FCW rectangles. */
		   if (g_odt_output.objectNum) {
			 int i = 0;
			 for (i = 0; i < g_odt_output.objectNum; i++) {
			   struct OdtObjectInfo *v = &g_odt_output.objects[i];
#ifdef ENABLE_RS_FACE
               int w = 1280;
			   int h = 720;
#else
			   int w = OdtWrapper_getCurrentImageWidth();
			   int h = OdtWrapper_getCurrentImageHeight();
#endif
			   struct win *ui_win = rk_fb_getuiwin();
			   if (ui_win) {
				 unsigned short *ui_buff = (unsigned short *)ui_win->buffer;
				 unsigned short rgb565_data;
				 rgb565_data = (COLOR_RED_R & 0x1f) << 11 | ((COLOR_RED_G & 0x3f) << 5) | (COLOR_RED_B & 0x1f);

				 int x0 = (v->x * ui_win->width) / w;
				 int y0 = (v->y * ui_win->height) / h;
				 int x1 = ((v->x + v->width) * ui_win->width) / w;
				 int y1 = ((v->y + v->height) * ui_win->height) / h;

				 printf("draw odt w,h(%d,%d) (%d,%d) - (%d,%d)\n", w, h, x0, y0, x1, y1);
				 show_rect_rgb565(ui_win->width, ui_win->height, ui_buff, x0, y0, x1, y1, rgb565_data);
				 show_rect_rgb565(ui_win->width, ui_win->height, ui_buff, x0 - 1, y0 - 1, x1 - 1, y1 - 1, rgb565_data);
				 show_rect_rgb565(ui_win->width, ui_win->height, ui_buff, x0 + 1, y0 + 1, x1 + 1, y1 + 1, rgb565_data);
			   }
			 }
		 }

}

void rec_event_callback(int cmd, void *msg0, void *msg1)
{
  //printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);
  switch (cmd)
  {
    case CMD_UPDATETIME:
      video_post_msg(hMainWnd, MSG_VIDEOREC, (WPARAM)msg0, EVENT_VIDEOREC_UPDATETIME);
	    break;
	  case CMD_ODT:
	  if (SetMode == MODE_RECORDING) {
		memcpy((void *)&g_odt_output, (void *)msg0, sizeof(g_odt_output));
		odtFlag = 1;
		//printf("MSG_ODT odtFlag=%d\n", odtFlag);
	  } else {
		memset(&g_odt_output, 0, sizeof(g_odt_output));
		odtFlag = 0;
	  }
      show_odt_result();
      break;
	  case CMD_PHOTOEND:
      video_post_msg(hMainWnd, MSG_PHOTOEND, (WPARAM)msg0, (LPARAM)msg1);
      break;
  }
}


void sd_event_callback(int cmd, void *msg0, void *msg1)
{
#ifdef RK_DEF_SDCARD

  //printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);
  switch (cmd)
  {
    case SD_NOTFIT:
      video_post_msg(hMainWnd, MSG_SDNOTFIT, (WPARAM)msg0, (LPARAM)msg1);
	  break;
	case SD_MOUNT_FAIL:
      video_post_msg(hMainWnd, MSG_SDMOUNTFAIL, (WPARAM)msg0, (LPARAM)msg1);
      break;
	case SD_CHANGE:
      video_post_msg(hMainWnd, MSG_SDCHANGE, (WPARAM)msg0, (LPARAM)msg1);
      break;
  }

#endif
}

void usb_event_callback(int cmd, void *msg0, void *msg1)
{
	//printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);
	video_post_msg(hMainWnd, MSG_USBCHAGE, (WPARAM)msg0, (LPARAM)msg1);
}

void batt_event_callback(int cmd, void *msg0, void *msg1)
{
	switch (cmd) {
	case CMD_UPDATE_CAP:
		video_post_msg(hMainWnd, MSG_BATTERY, (WPARAM)msg0, (LPARAM)msg1);
		break;
	case CMD_LOW_BATTERY: /* low power shutdown*/
		shutdown_deinit(hMainWnd);
		break;
	case CMD_DISCHARGE:
		video_post_msg(hMainWnd, MSG_BATTERY, (WPARAM)msg0, (LPARAM)msg1);
		shutdown_usb_disconnect(hMainWnd);
		break;
	default:
		break;
	}
}

int key_event_callback(int keycode, int keyval, int islongpress)
{
	if (keyval == KBD_VAL_KEY_DOWN && !islongpress)
		video_post_msg(hMainWnd, MSG_VIDEO_KEYDOWN, (WPARAM)keycode, (LPARAM)0);
	if (keyval == KBD_VAL_KEY_UP)
		video_post_msg(hMainWnd, MSG_VIDEO_KEYUP, (WPARAM)keycode, (LPARAM)0);
	if (islongpress)
		video_post_msg(hMainWnd, MSG_VIDEO_KEYLONGPRESS, (WPARAM)keycode, (LPARAM)0);
}

void poweroff_callback(int cmd, void *msg0, void *msg1)
{
	deinitrec(hMainWnd);
}

void mic_onoff(HWND hWnd, int onoff) {
#ifdef RK_DEF_AUDIO
	parameter_save_video_audio(onoff);
	video_record_setaudio(onoff);
#endif
}

int getSD(void) {
#ifdef RK_DEF_SDCARD
	return sdcard;
#else
	return 0;
#endif
}

/*SHUTDOWN FUNCTION*/
static int shutdown_deinit(HWND hWnd)
{
	if ((gshutdown == 1)||(gshutdown == 2)) {
		gshutdown = 2; //tell usb disconnect shutdown
		return -1;
	}
	gshutdown = 1;

	power_shutdown();

	return 0;
}

static int parking_suspend(HWND hWnd)
{
	int ret = 0;

	/* stop record video */
	//deinitrec(hWnd);

	ret = system("echo 4 >> /sys/class/graphics/fb0/blank");
	ret = system("echo mem >> /sys/power/state");

	if (ret == 0)
		printf("Power_Sleep success\n");
	else
		printf("Power_Sleep failure ret=%d\n", ret);

	return ret;
}

/* parking record video shutdown or suspend */
static int parking_record_process(int state)
{
	if (state == PARKING_SUSPEND) {
		printf("parking_record_process: suspend\n");
		parking_suspend(hMainWnd);
	} else if (state == PARKING_SHUTDOWN) {
		printf("parking_record_process: shutdown\n");
		shutdown_deinit(hMainWnd);
	}

	return 0;
}

/*usb disconenct shutdown process
 */
#ifdef USE_KEY_STOP_USB_DIS_SHUTDOWN
int stop_usb_disconnect_poweroff()
{
	gshutdown = 3;
	return 0;
}
#endif
int usb_disconnect_poweroff(void *arg)
{
	int i;
	HWND hWnd = (HWND)arg;

	for (i = 0; i < 10; i++) {
		if (charging_status_check()) {
			gshutdown = 0;
			printf("%s[%d]:vbus conenct,cancel shutdown\n",__func__, __LINE__);
			return 0;
		}
		if (gshutdown == 2)//powerkey or low battery shutdown
			break;
#ifdef USE_KEY_STOP_USB_DIS_SHUTDOWN
		if (gshutdown == 3) {
			//key stop usb disconnect shutdown
			gshutdown = 0;
			printf("%s[%d]:key stop usb disconnect shutdown\n",__func__, __LINE__);
			return 0;
		}
#endif
		printf("%s[%d]:shutdown wait---%d\n",__func__, __LINE__, i);
		sleep(1);
	}
	printf("%s[%d]:gshutdown=%d,shutdown...\n",__func__, __LINE__, gshutdown);
	#ifdef RK_DEF_SDCARD
	deinitrec(hWnd);
	#endif

	if (charging_status_check()) {
		gshutdown = 0;
		printf("%s[%d]:vbus conenct,cancel shutdown\n",__func__, __LINE__);
	#ifdef RK_DEF_SDCARD
		video_record_deinit();
		printf("init rec---\n");
		initrec(hWnd);
		if (!video_rec_state && sdcard == 1)
			startrec(hWnd);
	#endif
		return 0;
	}

	power_shutdown();
}

void *usb_disconnect_process(void *arg)
{
	printf ("usb_disconnect_process\n");
    usb_disconnect_poweroff(arg);
    pthread_exit(NULL);
}

pthread_t run_usb_disconnect(HWND hWnd)
{
    pthread_t tid;
	printf ("run_usb_disconnect\n");
    if(pthread_create(&tid, NULL, usb_disconnect_process, (void *)hWnd)) {
        printf("Create run_usb_disconnect thread failure\n");
        return -1;
    }

    return tid;
}

static int shutdown_usb_disconnect(HWND hWnd)
{
	if (gshutdown == 1)
		return -1;
	gshutdown = 1;

	run_usb_disconnect(hWnd);
	return 0;
}

/*
 *Get usb state interface
 */
static int get_usb_state()
{
	int fd, size;
	char path[] = "sys/class/android_usb/android0/state";
	char usb_state[15] = {0};

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf ("%s[%d]:Open %s failure\n",__func__, __LINE__, path);
		return -1;
	}

	size = read(fd, usb_state, 15);
	if (size < 0) {
		printf ("%s[%d]:Read %s failure\n",__func__, __LINE__, path);
		close(fd);
		return -1;
	}

	if (!strncmp(usb_state, "DISCONNECTED", 12)) {
		printf ("%s[%d]:---%s\n",__func__, __LINE__, usb_state);
		return 0;
	} else {
		printf ("%s[%d]:---%s\n",__func__, __LINE__, usb_state);
		return 1;
	}
}

// param == -1, format fail; param == 0, format success
int sdcardformat_back(void* arg, int param) {
#ifdef RK_DEF_SDCARD
  HWND hWnd = (HWND)arg;
  printf("%s\n", __func__);
  #ifdef MSG_FWK
    set_device_fault(FWK_MSG_GB28181_DEVICE_FAULT_SDCARD, 0);
  #endif

  if (wififormat_callback != NULL) {
    wififormat_callback();
    FlagForUI_ca.formatflag = 0;
    video_post_msg(hWnd, MSG_SDCHANGE, 0, 1);
    return 0;
  }
  
  if(param == 0)
    sdcard = 1;
  
  if (param < 0) {
    video_post_msg(hWnd, MSG_SDCARDFORMAT, param, EVENT_SDCARDFORMAT_FAIL);
  } else {
    set_sdcard_mount_state(1);
    video_post_msg(hWnd, MSG_SDCARDFORMAT, 0, EVENT_SDCARDFORMAT_FINISH);
  }
#else
  return 0;
#endif
}

int sdcardformat_notifyback(void* arg, int param) {
#ifdef RK_DEF_SDCARD
  HWND hWnd = (HWND)arg;
  printf("%s\n", __func__);
  if (wififormat_callback != NULL) {
    wififormat_callback();
    FlagForUI_ca.formatflag = 0;
    video_post_msg(hWnd, MSG_SDCHANGE, 0, 1);
    return 0;
  }
  if (param < 0)
    video_post_msg(hWnd, MSG_SDCARDFORMAT, param, EVENT_SDCARDFORMAT_FAIL);
  else {
    set_sdcard_mount_state(1);
    video_post_msg(hWnd, MSG_SDCARDFORMAT, 0, EVENT_SDCARDFORMAT_FINISH);
    video_post_msg(hWnd, MSG_SDCHANGE, 0, 1);
  }
#else
  return 0;
#endif
}

static void proc_MSG_FS_INITFAIL(HWND hWnd, int param);
int sdcardmount_back(void* arg, int param) {
#ifdef RK_DEF_SDCARD
  HWND hWnd = (HWND)arg;
  printf("sdcardmount_back param = %d\n", param);

  if(param == 0){
    video_post_msg(hWnd, MSG_REPAIR, 0, 1);
  } else if(param == 1) {
    video_post_msg(hWnd, MSG_FSINITFAIL, 0, 1);
  }
#else
  return 0;
#endif
}

//-------sys log
static int getResultFromSystemCall(const char* pCmd, char* pResult, int size) {
  int fd[2];
  if (pipe(fd)) {
    return -1;
  }
  // prevent content in stdout affect result
  fflush(stdout);
  // hide stdout
  int bak_fd = dup(STDOUT_FILENO);
  int new_fd = dup2(fd[1], STDOUT_FILENO);
  // the output of `pCmd` is write into fd[1]
  system(pCmd);
  printf("system!\n");
  read(fd[0], pResult, size - 1);
  printf("read!\n");
  pResult[strlen(pResult) - 1] = 0;

  // resume stdout
  dup2(bak_fd, new_fd);
  printf("--dup2!\n");

  return 0;
}

void ui_set_white_balance(int i) {
  pthread_mutex_lock(&set_mutex);
  video_record_set_white_balance(i);
  parameter_save_wb(i);
  pthread_mutex_unlock(&set_mutex);
}

void ui_set_exposure_compensation(int i) {
  pthread_mutex_lock(&set_mutex);
  video_record_set_exposure_compensation(i);
  parameter_save_ex(i);
  pthread_mutex_unlock(&set_mutex);
}

void ui_set_max_exposure_time(float i) {
  pthread_mutex_lock(&set_mutex);
  video_record_set_max_exposure_time(i);
  parameter_save_max_exposuretime(i);
  pthread_mutex_unlock(&set_mutex);
}

void ui_set_max_gain(float i) {
  pthread_mutex_lock(&set_mutex);
  video_record_set_max_gain(i);
  parameter_save_max_gain(i);
  pthread_mutex_unlock(&set_mutex);
}

void ui_set_night_en(char i) {
  pthread_mutex_lock(&set_mutex);
  video_record_set_night_en(i);
  parameter_save_night_en(i);
  pthread_mutex_unlock(&set_mutex);
}

void ui_set_daynight_mode(char i) {
  pthread_mutex_lock(&set_mutex);
  video_record_set_daynight_mode(i);
  parameter_save_daynight_mode(i);
  pthread_mutex_unlock(&set_mutex);
}


void ui_set_brightness(int i) {
  pthread_mutex_lock(&set_mutex);
  video_record_set_brightness(i);
  parameter_save_brightness(i);
  pthread_mutex_unlock(&set_mutex);
}

void ui_set_contrast(int i) {
  pthread_mutex_lock(&set_mutex);
  video_record_set_contrast(i);
  parameter_save_contrast(i);
  pthread_mutex_unlock(&set_mutex);
}

void ui_set_saturation(int i) {
  pthread_mutex_lock(&set_mutex);
  video_record_set_saturation(i);
  parameter_save_saturation(i);
  pthread_mutex_unlock(&set_mutex);
}

void ui_set_sharpness(int i) {
  pthread_mutex_lock(&set_mutex);
  video_record_set_sharpness(i);
  parameter_save_sharpness(i);
  pthread_mutex_unlock(&set_mutex);
}


int ui_camera_resolution(HWND hWnd, int resolution, int is_front)
{
	int i = 0;

	printf("ui_camera_resolution resolution=%d is_front=%d \n", resolution, is_front);

	pthread_mutex_lock(&set_mutex);
	video_record_get_front_resolution(frontcamera, 4);
	video_record_get_back_resolution(backcamera, 4);

	if(0 == resolution)
	{
		if(is_front)
		{
			for(i=0; i<4; i++)
			{
				if ((frontcamera[i].width == 1280) && (frontcamera[i].height == 720))
				{
					parameter_save_video_frontcamera_all(i, frontcamera[i].width, frontcamera[i].height, frontcamera[i].fps);
					parameter_save_vcamresolution(0);
					break;
				}
			}
		}
		else
		{
			for(i=0; i<4; i++)
			{
				if ((backcamera[i].width == 1280) && (backcamera[i].height == 720))
				{
					parameter_save_video_backcamera_all(i, backcamera[i].width, backcamera[i].height, backcamera[i].fps);
					break;
				}
			}
		}
	}
	else if(1 == resolution)
	{
		if(is_front)
		{
			for(i=0; i<4; i++)
			{
				if ((frontcamera[i].width == 1920) &&(frontcamera[i].height == 1080))
				{
					parameter_save_video_frontcamera_all(i, frontcamera[i].width, frontcamera[i].height, frontcamera[i].fps);
					parameter_save_vcamresolution(1);
					break;
				}
			}
		}
		else
		{
			for(i=0; i<4; i++)
			{
				if ((backcamera[i].width == 1920) && (backcamera[i].height == 1080))
				{
					parameter_save_video_backcamera_all(i, backcamera[i].width, backcamera[i].height, backcamera[i].fps);
					break;
				}
			}
		}
	}
	else
	{
		pthread_mutex_unlock(&set_mutex);
		return -1;
	}


	deinitrec(hWnd);
	video_record_deinit();
	initrec(hWnd);
	pthread_mutex_unlock(&set_mutex);

	return 0;
}

void deinit_init_camera(HWND hWnd) {
  pthread_mutex_lock(&set_mutex);
  printf("stoprec(hWnd)\n");
  deinitrec(hWnd);
  printf("video_record_deinit\n");
  video_record_deinit();
  printf("initrec\n");
  initrec(hWnd);
  printf("start rec\n");
  startrec(hWnd);
  printf("pthread_mutex_unlock(&set_mutex)\n");
  pthread_mutex_unlock(&set_mutex);
}

void ui_deinit_init_camera(HWND hWnd, char i, char j) {
  pthread_mutex_lock(&set_mutex);
  video_record_get_front_resolution(frontcamera, 4);
  video_record_get_back_resolution(backcamera, 4);
  if (i >= 0 && i < 4) {
    printf("parameter_save_video_frontcamera_all:%d %d %d %d\n", i,
           frontcamera[i].width, frontcamera[i].height, frontcamera[i].fps);
    parameter_save_video_frontcamera_all(
        i, frontcamera[i].width, frontcamera[i].height, frontcamera[i].fps);

    if ((frontcamera[i].width == 1280) && (frontcamera[i].height == 720)) {
      printf("parameter_save_vcamresolution(0)\n");
      parameter_save_vcamresolution(0);
    } else if ((frontcamera[i].width == 1920) &&
               (frontcamera[i].height == 1080)) {
      printf("parameter_save_vcamresolution(1)\n");
      parameter_save_vcamresolution(1);
    }
  }

  if (j >= 0 && j < 4) {
    parameter_save_video_backcamera_all(
        j, backcamera[j].width, backcamera[j].height, backcamera[j].fps);
  }

  /* Change setting resolution */
  storage_setting_event_callback(0, NULL, NULL);

  printf("stoprec(hWnd)\n");
  deinitrec(hWnd);
  printf("video_record_deinit\n");
  video_record_deinit();
  printf("initrec\n");
  initrec(hWnd);
  printf("pthread_mutex_unlock(&set_mutex)\n");
  pthread_mutex_unlock(&set_mutex);
}

void ui_save_vcamresolution_photo(HWND hWnd, char i) {
  parameter_save_vcamresolution_photo(i);
  ui_deinit_init_camera(hWnd, -1, -1);
}

static void cmd_IDM_RECOVERY(HWND hWnd) {
    parameter_recover();
    power_reboot();
}

static void cmd_IDM_FWVER(HWND hWnd) {
	printf("version %s\n", parameter_get_firmware_version());
}

void cmd_IDM_CAR(HWND hWnd, char i) {
  parameter_save_abmode(i);
  ui_deinit_init_camera(hWnd, -1, -1);
}

static void cmd_IDM_IDC(HWND hWnd, char i) {
  parameter_save_video_idc(i);
}

static void cmd_IDM_3DNR(HWND hWnd, char i) {
  parameter_save_video_3dnr(i);
}

static void cmd_IDM_ODT(HWND hWnd, WPARAM wParam) {
  switch (wParam) {
    case IDM_ODT_OFF:
      parameter_save_video_odt(0);
      video_odt_switch(0);
      FlagForUI_ca.adasflagtooff = 1;
      break;
    case IDM_ODT_ON:
      parameter_save_video_odt(1);
      video_odt_switch(1);
      break;
    default:
      break;
  }
}

static void cmd_IDM_detect(HWND hWnd, WPARAM wParam, LPARAM lParam) {
  printf("cmd_IDM_detect, wParam: %ld, lParam: %ld\n", wParam, lParam);
  switch (wParam) {
    case IDM_detectOFF:
      if (!lParam && parameter_get_video_de() == 0)
        return;
      video_record_stop_cache();
      stop_motion_detection();
      if (!lParam)
        parameter_save_video_de(0);
      break;
    case IDM_detectON:
      if (!lParam && parameter_get_video_de() == 1)
        return;
      if (!lParam)
        parameter_save_video_de(1);
        start_motion_detection(hWnd);
        video_record_start_cache(COLLISION_CACHE_DURATION);
      break;
    default:
      break;
  }
}

void cmd_IDM_frequency(char i) {
  parameter_save_video_fre(i);
  video_record_set_power_line_frequency(i);
}

static void cmd_IDM_FORMAT(HWND hWnd) {
#ifdef RK_DEF_SDCARD
	if (sdcard == 1) {
		fs_manage_format_sdcard(sdcardformat_back, (void*)hWnd, 0);
	}
#endif
}

static void cmd_IDM_BACKLIGHT(WPARAM wParam) {
  switch (wParam) {
    case IDM_BACKLIGHT_L:
      parameter_save_video_backlt(LCD_BACKLT_L);
      rk_fb_set_lcd_backlight(LCD_BACKLT_L);
      break;
    case IDM_BACKLIGHT_M:
      parameter_save_video_backlt(LCD_BACKLT_M);
      rk_fb_set_lcd_backlight(LCD_BACKLT_M);
      break;
    case IDM_BACKLIGHT_H:
      parameter_save_video_backlt(LCD_BACKLT_H);
      rk_fb_set_lcd_backlight(LCD_BACKLT_H);
      break;
    default:
      break;
  }
}

void cmd_IDM_VIDEO_QUALITY(HWND hWnd, unsigned int qualiy_level) {
  if (qualiy_level == parameter_get_bit_rate_per_pixel())
    return;
  if (video_rec_state)
    stoprec(hWnd);

  printf("change video quality level to < %d > [6: high,5: middle, 4: low]\n", qualiy_level);
  parameter_save_bit_rate_per_pixel(qualiy_level);
  video_record_reset_bitrate();
  #ifdef RK_DEF_SDCARD
  storage_setting_event_callback(0, NULL, NULL);
  #endif
}

/* Chinese index */
static uint32_t lic_chinese_idx = 0;
/* Charactor or number index */
static uint32_t lic_num_idx = 0;
static char licenseplate_str[20];
/* Save the car license plate */
static char licenseplate[8][3] = {"-","-","-","-","-","-","-","-",};
uint32_t licplate_pos[8] = {0};

#define PROVINCE_ABBR_MAX  37
#define LICENSE_CHAR_MAX   39

static const char g_license_chinese[PROVINCE_ABBR_MAX][3] = {
                " ","¾©","½ò","»¦","Óå","¼½","½ú","ÃÉ","ÁÉ","¼ª","ºÚ",
                "ËÕ","Õã","Íî","Ãö","¸Ó","Â³","Ô¥","¶õ","Ïæ","ÔÁ",
                "¹ð","Çí","´¨","¹ó","ÔÆ","²Ø","Çà","ÉÂ","¸Ê","Äþ",
                "ÐÂ","¸Û","°Ä","Ì¨","*","-"};

static const char g_license_char[LICENSE_CHAR_MAX][2] = {
               " ","0","1","2","3","4","5","6","7","8","9",
               "A","B","C","D","E","F","G","H","I","J",
               "K","L","M","N","O","P","Q","R","S","T",
               "U","V","W","X","Y","Z","*","-",};
void get_licenseplate_str(char (*lic_plate)[3], int len, char* licplate_str) {
  int i;

  if (licplate_str == NULL)
    return;

  if (parameter_get_licence_plate_flag() == 1) {
    for (i = 0; i < len; i++)
      strcat(licplate_str, (const char*)(lic_plate + i));
  }
}
static void get_licenseplate_and_pos() {
  int i, j;
  char** lic_plate = NULL;

  if (parameter_get_licence_plate_flag() == 1) {
      lic_plate = (char **)parameter_get_licence_plate();
      memcpy(licenseplate, lic_plate, sizeof(licenseplate));
  } else {
      for (i = 0; i < 8; i++)
        strcpy(*(licenseplate + i), "-");
  }

  /* Init the license plate position */
  memset(licplate_pos, 0, sizeof(licplate_pos));

  if (parameter_get_licence_plate_flag() != 1)
    return;

  for (j = 0; j < PROVINCE_ABBR_MAX; j++) {
    if (!strncmp(licenseplate[0], g_license_chinese[j], 2)) {
      licplate_pos[0] = j;
      break;
    }
  }

  for (i = 1; i < 8; i++) {
    for (j = 0; j < LICENSE_CHAR_MAX; j++) {
      if (!strncmp(licenseplate[i], g_license_char[j], 2)) {
        licplate_pos[i] = j;
        break;
      }
    }
  }
}
static void save_licenseplate() {
  uint32_t i, pos;
  uint32_t license_len;
  uint32_t show_license[MAX_TIME_LEN] = { 0 };

  memset(licenseplate_str, 0, sizeof(licenseplate_str));

  /* Save the content before change fouce CtrlID */
  if (lic_chinese_idx) {
    /* Save the fisrt Chinese Charactor */
    pos = licplate_pos[0];
    strcpy((char*)licenseplate[0], (char*)g_license_chinese[pos]);
    strcat(licenseplate_str, (char*)licenseplate[0]);
  }

  for (i = 1; i < 8; i++) {
    if (licplate_pos[i] != 0) {
      pos = licplate_pos[i];
      strcpy(licenseplate[i], g_license_char[pos]);
    }
    strcat(licenseplate_str + 2, (char*)licenseplate[i]);
  }

  parameter_save_licence_plate(licenseplate, 8);
  parameter_save_licence_plate_flag(1);

  watermark_get_show_license(show_license, licenseplate_str, &license_len);
  video_record_update_license(show_license, license_len);
}

void cmd_IDM_SetLicensePlate(HWND hWnd) {

}

static void cmd_IDM_DEBUG_PHOTO_ON(HWND hWnd) {
  if (!video_rec_state && sdcard == 1)
    startrec(hWnd);
  parameter_save_debug_photo(1);
}

void cmd_IDM_DEBUG_VIDEO_BIT_RATE(HWND hWnd, unsigned int val) {
  if (val == 0)
      val = 1;
  if (val == parameter_get_bit_rate_per_pixel())
    return;
  if (video_rec_state)
    stoprec(hWnd);
  printf("change bit rate per pixel to < %u >\n", val);
  parameter_save_bit_rate_per_pixel(val);
  video_record_reset_bitrate();
  #ifdef RK_DEF_SDCARD
  storage_setting_event_callback(0, NULL, NULL);
  #endif
}

static void cmd_IDM_USB(void) {
  parameter_save_video_usb(0);
  android_usb_config_ums();
  printf("usb mode \n");
}

static void cmd_IDM_ADB(void) {
  parameter_save_video_usb(1);
  android_usb_config_adb();
  printf("adb mode \n");
}

static void cmd_IDM_DEBUG_PHOTO_OFF(HWND hWnd) {
  if (video_rec_state != 0)
    stoprec(hWnd);
  parameter_save_debug_photo(0);
}

static void cmd_IDM_DEBUG_TEMP_ON(HWND hWnd) {
  parameter_save_debug_temp(1);
}

static void cmd_IDM_DEBUG_TEMP_OFF(HWND hWnd) {
  parameter_save_debug_temp(0);
}

//---------------MSG_COMMAND legacy cmd for ui---------------------
static int commandEvent(HWND hWnd, WPARAM wParam, LPARAM lParam) {
  struct ui_frame ui_720P = UI_FRAME_720P;
  struct ui_frame ui_1080P = UI_FRAME_1080P;
  printf("%s, wParam %d, lParam %d\n", __func__, wParam, lParam);
  switch (wParam) {
    case IDM_FONT_1:
      ui_deinit_init_camera(hWnd, 0, -1);
      break;
    case IDM_FONT_2:
      ui_deinit_init_camera(hWnd, 1, -1);
      break;
    case IDM_FONT_3:
      ui_deinit_init_camera(hWnd, 2, -1);
      break;
    case IDM_FONT_4:
      ui_deinit_init_camera(hWnd, 3, -1);
      break;
    case IDM_BACK_1:
      ui_deinit_init_camera(hWnd, -1, 0);
      break;
    case IDM_BACK_2:
      ui_deinit_init_camera(hWnd, -1, 1);
      break;
    case IDM_BACK_3:
      ui_deinit_init_camera(hWnd, -1, 2);
      break;
    case IDM_BACK_4:
      ui_deinit_init_camera(hWnd, -1, 3);
      break;
    case IDM_BOOT_OFF:
      parameter_save_debug_reboot(0);
      break;
    case IDM_RECOVERY_OFF:
      parameter_save_debug_recovery(0);
      break;
    case IDM_AWAKE_1_OFF:
      parameter_save_debug_awake(0);
      break;
    case IDM_STANDBY_2_OFF:
      parameter_save_debug_standby(0);
      break;
    case IDM_MODE_CHANGE_OFF:
      parameter_save_debug_modechange(0);
      break;
    case IDM_DEBUG_VIDEO_OFF:
      parameter_save_debug_video(0);
      break;
    case IDM_BEG_END_VIDEO_OFF:
      parameter_save_debug_beg_end_video(0);
      break;
    case IDM_DEBUG_PHOTO_OFF:
      cmd_IDM_DEBUG_PHOTO_OFF(hWnd);
      break;
    case IDM_DEBUG_TEMP_ON:
      cmd_IDM_DEBUG_TEMP_ON(hWnd);
      break;
    case IDM_DEBUG_TEMP_OFF:
	  cmd_IDM_DEBUG_TEMP_OFF(hWnd);
      break;
    case IDM_BOOT_ON:
      parameter_save_debug_reboot(1);
      break;
    case IDM_RECOVERY_ON:
      parameter_save_debug_recovery(1);
      break;
    case IDM_AWAKE_1_ON:
      parameter_save_debug_awake(1);
      break;
    case IDM_STANDBY_2_ON:
      parameter_save_debug_standby(1);
      break;
    case IDM_MODE_CHANGE_ON:
      parameter_save_debug_modechange(1);
      break;
    case IDM_DEBUG_VIDEO_ON:
      parameter_save_debug_video(1);
      break;
    case IDM_BEG_END_VIDEO_ON:
      parameter_save_debug_beg_end_video(1);
      break;
    case IDM_DEBUG_PHOTO_ON:
      cmd_IDM_DEBUG_PHOTO_ON(hWnd);
      break;
    case IDM_DEBUG_VIDEO_BIT_RATE1:
    case IDM_DEBUG_VIDEO_BIT_RATE2:
    case IDM_DEBUG_VIDEO_BIT_RATE4:
    case IDM_DEBUG_VIDEO_BIT_RATE5:
    case IDM_DEBUG_VIDEO_BIT_RATE6:
    case IDM_DEBUG_VIDEO_BIT_RATE8:
      cmd_IDM_DEBUG_VIDEO_BIT_RATE(hWnd, wParam - IDM_DEBUG_VIDEO_BIT_RATE);
      break;
    case IDM_VIDEO_QUALITY_H:
      cmd_IDM_VIDEO_QUALITY(hWnd,VIDEO_QUALITY_HIGH);
      break;
    case IDM_VIDEO_QUALITY_M:
      cmd_IDM_VIDEO_QUALITY(hWnd,VIDEO_QUALITY_MID);
      break;
    case IDM_VIDEO_QUALITY_L:
      cmd_IDM_VIDEO_QUALITY(hWnd,VIDEO_QUALITY_LOW);
      break;
    case IDM_LICENSEPLATE_WATERMARK:
      cmd_IDM_SetLicensePlate(hWnd);
      break;      
    case IDM_1M_ph:
      ui_save_vcamresolution_photo(hWnd, 0);
      break;
    case IDM_2M_ph:
      ui_save_vcamresolution_photo(hWnd, 1);
      break;
    case IDM_3M_ph:
      ui_save_vcamresolution_photo(hWnd, 2);
      break;
    case IDM_RECOVERY:
      cmd_IDM_RECOVERY(hWnd);
      break;
    case IDM_FWVER:
      cmd_IDM_FWVER(hWnd);
      break;
    case IDM_USB:
      cmd_IDM_USB();
      break;
    case IDM_ADB:
      cmd_IDM_ADB();
      break;
    case IDM_ABOUT_CAR:
      break;
    case IDM_ABOUT_TIME:
      break;
    case IDM_1MIN:
      parameter_save_recodetime(60);
      storage_setting_event_callback(0, NULL, NULL);
      break;
    case IDM_3MIN:
      parameter_save_recodetime(180);
      storage_setting_event_callback(0, NULL, NULL);
      break;
    case IDM_5MIN:
      parameter_save_recodetime(300);
      storage_setting_event_callback(0, NULL, NULL);
      break;
    case IDM_CAR1:
      cmd_IDM_CAR(hWnd, 0);
      break;
    case IDM_CAR2:
      cmd_IDM_CAR(hWnd, 1);
      break;
    case IDM_CAR3:
      cmd_IDM_CAR(hWnd, 2);
      break;
    case IDM_IDCOFF:
      cmd_IDM_IDC(hWnd, 0);
      break;
    case IDM_IDCON:
      cmd_IDM_IDC(hWnd, 1);
      break;
    case IDM_3DNROFF:
      cmd_IDM_3DNR(hWnd, 0);
      break;
    case IDM_3DNRON:
      cmd_IDM_3DNR(hWnd, 1);
      break;
    case IDM_ODT_OFF:
      cmd_IDM_ODT(hWnd, wParam);
      break;
    case IDM_ODT_ON:
      cmd_IDM_ODT(hWnd, wParam);
      break;
    case IDM_bright1:
      ui_set_white_balance(0);
      break;
    case IDM_bright2:
      ui_set_white_balance(1);
      break;
    case IDM_bright3:
      ui_set_white_balance(2);
      break;
    case IDM_bright4:
      ui_set_white_balance(3);
      break;
    case IDM_bright5:
      ui_set_white_balance(4);
      break;
    case IDM_exposal1:
      ui_set_exposure_compensation(0);
      break;
    case IDM_exposal2:
      ui_set_exposure_compensation(1);
      break;
    case IDM_exposal3:
      ui_set_exposure_compensation(2);
      break;
    case IDM_exposal4:
      ui_set_exposure_compensation(3);
      break;
    case IDM_exposal5:
      ui_set_exposure_compensation(4);
      break;
    case IDM_detectON:
    case IDM_detectOFF:
//#if ENABLE_MD_IN_MENU
      cmd_IDM_detect(hWnd, wParam, lParam);
//#endif
      break;
    case IDM_markOFF:
      parameter_save_video_mark(0);
      break;
    case IDM_markON:
      parameter_save_video_mark(1);
      break;
    case IDM_recordOFF:
      mic_onoff(hWnd, 0);
      break;
    case IDM_recordON:
      mic_onoff(hWnd, 1);
      break;
    case IDM_AUTOOFFSCREENON:
      screenoff_time = 15;
      parameter_save_screenoff_time(15);  // 15S
      break;
    case IDM_AUTOOFFSCREENOFF:
      screenoff_time = -1;
      parameter_save_screenoff_time(-1);
      break;
    case IDM_autorecordOFF:
      parameter_save_video_autorec(0);
      break;
    case IDM_autorecordON:
	#ifdef RK_DEF_SDCARD
      parameter_save_video_autorec(1);
	#endif
	  break;
    case IDM_WIFIOFF:
	#ifdef RK_DEF_WIFI
      parameter_save_wifi_en(0);
      wifi_management_stop();
	#endif
      break;
    case IDM_WIFION:
	#ifdef RK_DEF_WIFI
      parameter_save_wifi_en(1);
      wifi_management_start();
	#endif
      break;
    case IDM_50HZ:
      cmd_IDM_frequency(CAMARE_FREQ_50HZ);
      break;
    case IDM_60HZ:
      cmd_IDM_frequency(CAMARE_FREQ_60HZ);
      break;
    case IDM_1M:
      SetCameraMP = 0;
      break;
    case IDM_2M:
      SetCameraMP = 1;
      break;
    case IDM_3M:
      SetCameraMP = 2;
      break;
    case IDM_FORMAT:
      cmd_IDM_FORMAT(hWnd);
      break;
    case IDM_SETDATE:
      //cmd_IDM_SETDATE();
      break;
    case IDM_BACKLIGHT_L:
      cmd_IDM_BACKLIGHT(wParam);
      break;
    case IDM_BACKLIGHT_M:
      cmd_IDM_BACKLIGHT(wParam);
      break;
    case IDM_BACKLIGHT_H:
      cmd_IDM_BACKLIGHT(wParam);
      break;
    case IDM_ABOUT_SETTING:
      break;
    case IDM_COLLISION_NO:
      parameter_save_collision_level(COLLI_CLOSE);
      video_record_stop_cache();
      collision_unregister();
      break;
    case IDM_COLLISION_L:
      parameter_save_collision_level(COLLI_LEVEL_L);
      collision_register();
      video_record_start_cache(COLLISION_CACHE_DURATION);
      break;
    case IDM_COLLISION_M:
      parameter_save_collision_level(COLLI_LEVEL_M);
      collision_register();
      video_record_start_cache(COLLISION_CACHE_DURATION);
      break;
    case IDM_COLLISION_H:
      parameter_save_collision_level(COLLI_LEVEL_H);
      collision_register();
      video_record_start_cache(COLLISION_CACHE_DURATION);
      break;
    case IDM_LEAVEREC_OFF:
      parameter_save_leavecarrec(0);
      parking_unregister();
      break;
    case IDM_LEAVEREC_ON:
      parameter_save_leavecarrec(1);
      parking_register();
      break;
    /* TODO: show status of time-lapse by icon in screen */
    case IDM_TIME_LAPSE_OFF:
      parameter_save_time_lapse_interval(0);
      break;
    case IDM_TIME_LAPSE_INTERNAL_1s:
      parameter_save_time_lapse_interval(1);
      break;
    case IDM_TIME_LAPSE_INTERNAL_5s:
      parameter_save_time_lapse_interval(5);
      break;
    case IDM_TIME_LAPSE_INTERNAL_10s:
      parameter_save_time_lapse_interval(10);
      break;
    case IDM_TIME_LAPSE_INTERNAL_30s:
      parameter_save_time_lapse_interval(30);
      break;
    case IDM_TIME_LAPSE_INTERNAL_60s:
      parameter_save_time_lapse_interval(60);
      break;      
    return 0;
  }

  return 1;
}

static int startrec0(HWND hWnd) {
#ifdef RK_DEF_SDCARD
  int ret;
  if (video_rec_state)
    return 0;

  // gc_sd_space();
  // test code start
  #ifdef RK_DEF_AUDIO
  audio_sync_play("/usr/local/share/sounds/VideoRecord.wav");
  #endif
  // test code end
  ret = video_record_startrec();
  if (ret == 0) {
    video_rec_state = true;
    FlagForUI_ca.video_rec_state_ui = video_rec_state;
    sec = 0;
  }
  return ret;
#else
  return -1;
#endif
}

int startrec(HWND hWnd) {
#ifdef RK_DEF_SDCARD
  if(!sdcard) {
    printf("startrec: sdcard not found!\n");
    return -1;
  }  
  
  return startrec0(hWnd);
#else
  return -1;
#endif
}


static int stoprec0(HWND hWnd) {
#ifdef RK_DEF_SDCARD
  if (!video_rec_state)
    return 0;
  video_rec_state = false;
  FlagForUI_ca.video_rec_state_ui = video_rec_state;

  video_record_stoprec();
#endif
  return 0;
}

int stoprec(HWND hWnd) {
#ifdef RK_DEF_SDCARD
  return stoprec0(hWnd);
#else
  return -1;
#endif
}

int force_stoprec(HWND hWnd) {
  set_motion_detection_trigger_value(false);

  return stoprec0(hWnd);
}

static void initrec(HWND hWnd) {
#ifdef RK_DEF_SDCARD
  struct ui_frame front = {parameter_get_video_frontcamera_width(),
                           parameter_get_video_frontcamera_height(),
                           parameter_get_video_frontcamera_fps()};

  struct ui_frame back = {parameter_get_video_backcamera_width(),
                          parameter_get_video_backcamera_height(),
                          parameter_get_video_backcamera_fps()};

  char photo_size = parameter_get_vcamresolution_photo();
  if (photo_size < 0 || photo_size >= 3)
    photo_size = 0;
  struct ui_frame photo[3] = {UI_FRAME_720P, UI_FRAME_1080P, UI_FRAME_1080P};

  memset(&g_odt_output, 0, sizeof(g_odt_output));

  if (SetMode != MODE_PHOTO) {
    video_record_set_record_mode(true);
    video_record_init(&front, &back);
  } else {
    video_record_set_record_mode(false);
    video_record_init(&photo[photo_size], &photo[photo_size]);
  }

  user_entry();  // test code
  if (parameter_get_video_de() == 1)
    video_post_msg(hWnd, MSG_VIDEO_COMMAND, IDM_detectON, 1);
  if (parameter_get_collision_level() != 0)
    video_record_start_cache(COLLISION_CACHE_DURATION);
#endif
}

static void deinitrec(HWND hWnd) {
#ifdef RK_DEF_SDCARD
  /*
   * FIXME: I do not think it is a good idea that
   * manage wifi related business in UI thread,
   * too trivial.
   */
  video_record_stop_ts_transfer(1);

  video_record_stop_cache();
  stop_motion_detection();
  user_exit();  // test code
  if (video_rec_state)
    stoprec(hWnd);
#endif
}

static void initrec_slp_out()
{
#ifdef RK_DEF_SDCARD
  struct ui_frame front = {parameter_get_video_frontcamera_width(),
                           parameter_get_video_frontcamera_height(),
                           parameter_get_video_frontcamera_fps()};

  struct ui_frame back = {parameter_get_video_backcamera_width(),
                          parameter_get_video_backcamera_height(),
                          parameter_get_video_backcamera_fps()};

  char photo_size = parameter_get_vcamresolution_photo();
  if (photo_size < 0 || photo_size >= 3)
    photo_size = 0;
  struct ui_frame photo[3] = {UI_FRAME_720P, UI_FRAME_1080P, UI_FRAME_1080P};

  if (SetMode != MODE_PHOTO) {
    video_record_set_record_mode(true);
    video_record_init(&front, &back);
  } else {
    video_record_set_record_mode(false);
    video_record_init(&photo[photo_size], &photo[photo_size]);
  }
#endif
}

#ifdef RK_DEF_SDCARD
int record_get_state(void *ptr)
{
  return 1;
}

int record_slp_out(void *ptr, int state)
{
  if (state)
    initrec_slp_out();

  return 0;
}

int record_slp_in(void *ptr)
{
  video_record_stop_ts_transfer(1);
  video_record_stop_cache();
  stop_motion_detection();
  video_record_deinit();
  return 0;
}
#endif

static void proc_MSG_USBCHAGE(HWND hWnd,
                              int message,
                              WPARAM wParam,
                              LPARAM lParam) {
  int usb_sd_chage = (int)lParam;

  printf("MSG_USBCHAGE : usb_sd_chage = ( %d )\n", usb_sd_chage);

  if (usb_sd_chage == 1) {
	printf("usb_sd_chage == 1:USB_MODE==0\n");
	if (sdcard == 1)
		cvr_usb_sd_ctl(USB_CTRL, 1);
	 else if (sdcard == 0)
		printf("no SD card insertted!!!\n");
  } else if (usb_sd_chage == 0) {
    cvr_usb_sd_ctl(USB_CTRL, 0);
    printf("usb_sd_chage == 0:USB_MODE==0\n");
  }
}

static proc_record_SCANCODE_CURSORBLOCKRIGHT(HWND hWnd) {
#ifdef USE_CIF_CAMERA
  short inputid = parameter_get_cif_inputid();
  inputid = (inputid == 0) ? 1 : 0;
  parameter_save_cif_inputid(inputid);
  ui_deinit_init_camera(hWnd, -1, -1);
#endif

  video_record_inc_nv12_raw_cnt();
}

int ui_takephoto(HWND hWnd) {
	int ret = 0;
    printf("takephoto = %d\n", !takephoto);
  if (!takephoto) {
    takephoto = true;
#ifdef RK_DEF_AUDIO
    audio_play(capture_sound);
#endif
    if (video_record_takephoto() == -1) {
      takephoto = false;
	  ret = -1;
    }
  }
  else
  {
  	ret = -1;
  }
  takephoto = false;
  return ret;
}

static void proc_MSG_SDMOUNTFAIL(HWND hWnd)
{
#ifdef RK_DEF_SDCARD
	int mesg = 0;

	printf("MSG_SDMOUNTFAIL\n");
#ifdef MSG_FWK
	set_device_fault(FWK_MSG_GB28181_DEVICE_FAULT_SDCARD, 1);
	protocol_reverse_message_alarm_sdcard(1, SDCARD_ERR_01);
#endif

#endif
}

static void proc_MSG_SDNOTFIT(HWND hWnd)
{
#ifdef RK_DEF_SDCARD

	int mesg = 0;

	printf("MSG_SDNOTFIT\n");
#ifdef MSG_FWK
	set_device_fault(FWK_MSG_GB28181_DEVICE_FAULT_SDCARD, 1);
	protocol_reverse_message_alarm_sdcard(1, SDCARD_ERR_02);
#endif

#endif
}

static void proc_MSG_FS_INITFAIL(HWND hWnd, int param)
{
#ifdef RK_DEF_SDCARD

  int mesg = 0;

  printf("FS_INITFAIL\n");

#ifdef MSG_FWK
	/* no space */
	if (param == -1) {
		set_device_fault(FWK_MSG_GB28181_DEVICE_FAULT_SDCARD, 1);
		protocol_reverse_message_alarm_sdcard(1, SDCARD_ERR_03);
	/* init fail */
	} else if (param == -2) {
		set_device_fault(FWK_MSG_GB28181_DEVICE_FAULT_SDCARD, 1);
		protocol_reverse_message_alarm_sdcard(1, SDCARD_ERR_04);
	/* else error*/
	} else {
		set_device_fault(FWK_MSG_GB28181_DEVICE_FAULT_SDCARD, 1);
		protocol_reverse_message_alarm_sdcard(1, SDCARD_ERR_05);
	}
#endif

#endif

	return;
}

static void  proc_MSG_SDCARDFORMATFAIL(HWND hWnd, int ret)
{
#ifdef RK_DEF_SDCARD

	int mesg = 0;

	printf("MSG_SDNOTFIT\n");
#ifdef MSG_FWK
	if (ret = -5) {
		set_device_fault(FWK_MSG_GB28181_DEVICE_FAULT_SDCARD, 1);
		protocol_reverse_message_alarm_sdcard(1, SDCARD_ERR_08);
	} else {
		set_device_fault(FWK_MSG_GB28181_DEVICE_FAULT_SDCARD, 1);
		protocol_reverse_message_alarm_sdcard(1, SDCARD_ERR_06);
	}
#endif

#endif
}


int fb_init(void)
{
  struct win * ui_win;
  struct color_key color_key;
  unsigned short rgb565_data;
  unsigned short *ui_buff;
  int i;

  rk_fb_init(FB_FORMAT_RGB_565);
  ui_win = rk_fb_getuiwin();
  ui_buff = (unsigned short *)ui_win->buffer;
  g_ui_win = ui_win;

  //enable and set color key
  color_key.enable = 1;
  color_key.red = (COLOR_KEY_R & 0x1f) << 3;
  color_key.green = (COLOR_KEY_G & 0x3f) << 2;
  color_key.blue = (COLOR_KEY_B & 0x1f) << 3;
  rk_fb_set_color_key(color_key);

#ifdef ENABLE_RS_FACE
  g_ui_clear_buffer = malloc(ui_win->width * ui_win->height*2);
  ui_buff = (unsigned short *)g_ui_clear_buffer;
#endif

  //set ui win color key
  rgb565_data = (COLOR_KEY_R & 0x1f) << 11 | ((COLOR_KEY_G & 0x3f) << 5) | (COLOR_KEY_B & 0x1f);
  for (i = 0; i < ui_win->width * ui_win->height; i ++) {
  	ui_buff[i] = rgb565_data;
  }

#ifdef ENABLE_RS_FACE
  ui_buff = (unsigned short *)ui_win->buffer;
  memcpy((void *)ui_buff, g_ui_clear_buffer, ui_win->width * ui_win->height*2);
#endif

  //disp ui image
  rk_fb_ui_disp(ui_win);

  return 0;
}

#ifdef ENABLE_ODT_DEBUG
static void drawOdtObjects(HDC hdc)
{
    int i = 0;
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
    struct win * ui_win;
    unsigned short *ui_buff;
    unsigned short rgb565_data;

    if (!g_ui_win)
      g_ui_win = rk_fb_getuiwin();
    ui_win = g_ui_win;

    if (!ui_win)
      return;

    ui_buff = (unsigned short *)ui_win->buffer;

    int w = OdtWrapper_getCurrentImageWidth();
    int h = OdtWrapper_getCurrentImageHeight();

    OdtObjectInfos objectInfos;
    OdtWrapper_getDebugObjects(&objectInfos);

    for (i = 0; i < objectInfos.objectNum; i++) {

        OdtObjectInfo *objectInfo = &objectInfos.objects[i];
        rgb565_data = (COLOR_GREEN_R & 0x1f) << 11 | ((COLOR_GREEN_G & 0x3f) << 5) | (COLOR_GREEN_B & 0x1f);

        x0 = (objectInfo->x * ui_win->width) / w;
        y0 = (objectInfo->y * ui_win->height) / h;
        x1 = ((objectInfo->x + objectInfo->width) * ui_win->width) / w;
        y1 = ((objectInfo->y + objectInfo->height) * ui_win->height) / h;

        printf("drawOdtObjects (%d,%d) - (%d,%d)\n", x0, y0, x1, y1);
        show_rect_rgb565(ui_win->width, ui_win->height, ui_buff, x0, y0, x1, y1, rgb565_data);
        show_rect_rgb565(ui_win->width, ui_win->height, ui_buff, x0 - 1, y0 - 1, x1 - 1, y1 - 1, rgb565_data);
        show_rect_rgb565(ui_win->width, ui_win->height, ui_buff, x0 + 1, y0 + 1, x1 + 1, y1 + 1, rgb565_data);

    }
}
#endif

static int main_msg_proc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam) {
	char buff2[20];
	RECT rc;
	char msg_text[30];
	int mesg = 0;

  //fprintf( stderr, "message %d, wParam =%d ,lParam =%d \n", message, (int)wParam, (int)lParam);
  switch (message) {
    case MSG_HDMI:
#if HAVE_DISPLAY
      if (lParam == 1) {
        rk_fb_set_out_device(OUT_DEVICE_HDMI);
      } else {
        rk_fb_set_out_device(OUT_DEVICE_LCD);
      }
#endif
      send_msg_to_ui(message, wParam, lParam);
      break;
    case MSG_MOUSE:
        send_msg_to_ui(message, wParam, lParam);
        break;
    case MSG_REPAIR:
      printf("Sd card is damaged in video, \nwe have repair!\n");
      send_msg_to_ui(message, wParam, lParam);
      break;
    case MSG_FSINITFAIL:
      proc_MSG_FS_INITFAIL(hWnd, -1);
      //send_msg_to_ui(message, wParam, lParam);
      break;
    case MSG_PHOTOEND:
      if (lParam == 1 && takephoto) {
        takephoto = false;
      }
      send_msg_to_ui(message, wParam, lParam);
      break;
    case MSG_ODT:
      break;
    case MSG_SDMOUNTFAIL:
      proc_MSG_SDMOUNTFAIL(hWnd);
      //send_msg_to_ui(message, wParam, lParam);
      break;
    case MSG_SDNOTFIT:
      proc_MSG_SDNOTFIT(hWnd);
      //send_msg_to_ui(message, wParam, lParam);
      break;
    case MSG_SDCARDFORMAT:
      if (lParam == EVENT_SDCARDFORMAT_FINISH) {
        printf("sd card format finish\n");
      } else if (lParam == EVENT_SDCARDFORMAT_FAIL) {
		proc_MSG_SDCARDFORMATFAIL(hWnd, (int)wParam);
      }
      //send_msg_to_ui(message, wParam, lParam);      
      break;
#ifdef NTP_SYNC_REQUEST
    case MSG_NTP_SYNCED:
      if (lParam == EVENT_NTP_SYNC_FINISH) {
        printf("ntp sync time finish\n");
      } else if (lParam == EVENT_NTP_SYNC_FAIL) {
        printf("ntp sync time fail\n");
          if(0 == ntp_wait_for_sync(ntp_synced_callback, (void *)hWnd, 0)) {
            /*video_post_msg(hWnd, MSG_SDCHANGE, 0, 1);**/
            if (SetMode == MODE_RECORDING) {
              DBGMSG(CI,("SetMode == MODE_RECORDING\n"));
              if (sdcard == 1) {
                DBGMSG(CI,("sdcard == 1\n"));
                if (parameter_get_video_autorec())
                  startrec(hWnd);
            }
          }
        }
      }
      break;
#endif
    case MSG_VIDEOREC:
      if (lParam == EVENT_VIDEOREC_UPDATETIME) {
        sec = (int)wParam;
      }
      send_msg_to_ui(message, wParam, lParam);
      break;
    case MSG_CAMERA: {
      // printf("%s wParam = %s, lParam = %d\n", __func__, wParam, lParam);
      if (SetMode != MODE_RECORDING && SetMode != MODE_PHOTO)
        break;
      if (lParam == 1) {
        struct ui_frame front = {parameter_get_video_frontcamera_width(),
                                 parameter_get_video_frontcamera_height(),
                                 parameter_get_video_frontcamera_fps()};

        struct ui_frame back = {parameter_get_video_backcamera_width(),
                                parameter_get_video_backcamera_height(),
                                parameter_get_video_backcamera_fps()};

        usleep(200000);
        video_record_addvideo(wParam, &front, &back, video_rec_state, 1);
      } else {
        video_record_deletevideo(wParam);
      }
      break;
    }
    case MSG_ATTACH_USER_MUXER: {
	#ifdef RK_DEF_SDCARD
      video_record_attach_user_muxer((void *)wParam, (void *)lParam, 1);
	#endif
      break;
    }
    case MSG_ATTACH_USER_MDPROCESSOR: {
	#ifdef RK_DEF_SDCARD
      video_record_attach_user_mdprocessor((void *)wParam, (void *)lParam);
	#endif
      break;
    }
    case MSG_RECORD_RATE_CHANGE: {
      printf("!!! lParam: %d, video_rec_state: %d\n", (bool)lParam, video_rec_state);
      if (!(bool)lParam && sdcard == 1) {
        //startrec0(hWnd);
        //video_record_rate_change((void *)wParam, (int)lParam);
        #ifdef RK_DEF_SDCARD
        video_record_savefile();
		#endif
        break;
      }
      //video_record_rate_change((void *)wParam, (int)lParam);
      break;
    }
    case MSG_COLLISION: {
	#ifdef RK_DEF_SDCARD
     if (SetMode == MODE_RECORDING && sdcard == 1)
       video_record_savefile();
	#endif
     break;
    }
    case MSG_FILTER: {
      // filterForUI= (int)lParam;
      break;
    }

    case MSG_USBCHAGE:
      /* proc_MSG_USBCHAGE(hWnd, message, wParam, lParam); */
      break;
    case MSG_SDCHANGE:
	#ifdef RK_DEF_SDCARD
      sdcard = (int)lParam;
	  FlagForUI_ca.sdcard_ui = sdcard;
      printf("MSG_SDCHANGE sdcard = %d\n", sdcard);
      if (sdcard == 1) {
        int ret = fs_manage_init(sdcardmount_back, (void*)hWnd);
        if (ret < 0) {
          sdcard = 0;
          video_record_stop_savecache();
          proc_MSG_FS_INITFAIL(hWnd, ret);
          break;
        }
        if (parameter_get_video_de() == 1) {
          video_post_msg(hMainWnd, MSG_VIDEO_COMMAND, IDM_detectON, 1);
        }
        /* Set the filename last time */
        storage_check_timestamp();
        video_record_get_user_noise();
      } else {
        #ifdef MSG_FWK
		  set_device_fault(FWK_MSG_GB28181_DEVICE_FAULT_SDCARD, 0);
        #endif
        if (parameter_get_video_de() == 1) {
          video_post_msg(hMainWnd, MSG_VIDEO_COMMAND, IDM_detectOFF, 1);
        }
        video_record_stop_savecache();
      }
#ifdef NTP_SYNC_REQUEST
      if(ntp_wait_for_sync(ntp_synced_callback, (void *)hWnd, 0)) {
        break;
      }
#endif
      if (SetMode == MODE_RECORDING) {
        if (sdcard == 1) {
          if (parameter_get_video_autorec()) {
            startrec(hWnd);
          }
        } else {
          force_stoprec(hWnd);
        }
      } 

      if (sdcard == 0) {
        fs_manage_deinit();
      }
	#endif
      break;

    case MSG_BATTERY:
		cap = lParam;
		if (cap > 100)
			battery = 0;
		else if ((cap >= 0) && (cap < (25 - BATTERY_CUSHION)))
			battery = 1;
		else if ((cap >= (25 + BATTERY_CUSHION)) && (cap < (50 - BATTERY_CUSHION)))
			battery = 2;
		else if ((cap >= (50 + BATTERY_CUSHION)) && (cap < (75 - BATTERY_CUSHION)))
			battery = 3;
		else if (cap >= (75 + BATTERY_CUSHION))
			battery = 4;
		else {
			if ((battery != 0) && (last_battery != 0))
				battery = last_battery;
			if (battery == 0) {
				if ((cap >= (25 - BATTERY_CUSHION)) && (cap < (25 + BATTERY_CUSHION)))
					battery  = 1;
				if ((cap >= (50 - BATTERY_CUSHION)) && (cap < (50 + BATTERY_CUSHION)))
					battery  = 2;
				if ((cap >= (75 - BATTERY_CUSHION)) && (cap < (75 + BATTERY_CUSHION)))
					battery  = 3;
			}
		}
		last_battery = battery;

		//printf("battery level=%d\n", battery);
		break;

    case MSG_VIDEO_TIMER:
		/* gparking_gs_active (0: gsensor negative 1: gsneosr active) */
		gparking = parameter_get_leavecarrec();
		if ((gparking_gs_active == 0) && (PARKING_ENABLE == gparking)) {
			if ((SetMode == MODE_RECORDING) &&
			    (video_rec_state != 0))
				gparking_count++;
			else
				gparking_count = 0;
			printf("gparking_count = %d\n", gparking_count);
			if (gparking_count >= PARKING_RECORD_COUNT) {
				printf("shutdown or suspend:\n");
				parking_record_process(PARKING_SHUTDOWN);
			}
		}
		if (((gparking_gs_active == 1) && (PARKING_ENABLE == gparking))
			||(PARKING_ENABLE != gparking)) {
			gparking_count = 0;
			gparking_gs_active = 0;
		}

#ifdef USE_WATCHDOG
		if (fd_wtd != -1)
			ioctl(fd_wtd, WDIOC_KEEPALIVE, 0);
#endif

#ifdef RK_DEF_SDCARD
		if (SetMode == MODE_RECORDING)
			video_record_fps_count();
#endif

	  
#ifdef USE_WATERMARK
		if (((SetMode == MODE_RECORDING) || (SetMode == MODE_PHOTO)) &&
			parameter_get_video_mark())
		{
	            char systime[22];
	            time_t ltime;
	            struct tm* today;
	            time(&ltime);
	            today = localtime(&ltime);
	            sprintf(systime, "%04d-%02d-%02d %02d:%02d:%02d\n",
	                    1900 + today->tm_year, today->tm_mon + 1, today->tm_mday,
	                    today->tm_hour, today->tm_min, today->tm_sec);

	            uint32_t show_time[MAX_TIME_LEN] = { 0 };
	            watermark_get_show_time(show_time, today, MAX_TIME_LEN);
			#ifdef RK_DEF_SDCARD
	            video_record_update_time(show_time, MAX_TIME_LEN);
			#endif

                /* Show license plate */
                if (parameter_get_licence_plate_flag()) {
                  if (licenseplate_str[0] == 0) {
                    get_licenseplate_str((char(*)[3])parameter_get_licence_plate(), 8, licenseplate_str);
                    get_licenseplate_and_pos();
                  }
                }              
		}
#endif


    if (FlagForUI_ca.adasflagtooff == 1) {
      printf("adasflagtooff");
      /* clear background */
      fb_clear_ui();
      FlagForUI_ca.adasflagtooff = 0;
    }

#ifdef ENABLE_ODT_DEBUG
    if (SetMode == MODE_RECORDING)
      drawOdtObjects(hdc);
#endif

		if (video_rec_state) {
			char strtime[20];
			sprintf(strtime, "%02ld:%02ld:%02ld", (long int)(sec / 3600),
				(long int)((sec / 60) % 60), (long int)(sec % 60));
			collision_rec_time = (long int)(sec % 60);
		}
		break;
	case MSG_VIDEO_COMMAND:
		commandEvent(hWnd, wParam, lParam);
		break;
	case MSG_VIDEO_KEYDOWN:
		printf("%s MSG_VIDEO_KEYDOWN key = %d\n", __func__, wParam);
#if LONG_PRESS_RECOVERY_METHOD == 1
		gettimeofday(&key_status.last_press, NULL);
		key_status.key_value = wParam;
#endif
		break;
	case MSG_VIDEO_KEYUP:
		printf("%s MSG_VIDEO_KEYUP key = %d\n", __func__, wParam);
#if LONG_PRESS_RECOVERY_METHOD == 1
		key_status.key_value = 0;
#endif
		break;

	case MSG_VIDEO_KEYLONGPRESS:
		printf("%s MSG_VIDEO_KEYLONGPRESS key = %d key_status.key_value=%d \n", __func__, wParam, key_status.key_value);
#if LONG_PRESS_RECOVERY_METHOD == 1
		{
			struct timeval tv;
			double time_usec;
			if( RECOVERY_KEY == key_status.key_value) {
				gettimeofday(&tv, NULL);
				time_usec = tvdiff(&key_status.last_press, &tv);
				if(/*time_usec > RECOVERY_PRESS_TIME*/1) {
					printf("recovery me now,press key time %f\n", time_usec);
					//parameter_recover();
					//power_reboot();
					video_mfnr_capture_req();
				}
			}
			else if( VOICE_KEY == key_status.key_value) {
				printf("VOICE_KEY is long press! \n");
				//DO STH
			}
		}
#elif LONG_PRESS_RECOVERY_METHOD == 2/* directly recovery */
		if (wParam == RECOVERY_KEY) {
			printf("recovery me now\n");
			parameter_recover();
			power_reboot();
		}
#endif
    case MSG_VIDEO_GET_PARAMS:
      {
        printf("main: get MSG_VIDEO_GET_PARAMS\n");
        #if 0
        const struct sys_param *param;
        param = param_get_sys_param();
        send_msg_ex_to_ui(MSG_VIDEO_GET_PARAMS, 0, 0, param, sizeof(struct sys_param));
        #endif
      }
      break;
	}
	return 0;
}

int get_odt_result_count()
{
	return g_odt_output.objectNum;
}

int ui_set_odt(HWND hWnd, int on_off){
	int ret = 0;
	if(on_off){
		cmd_IDM_ODT(hWnd, IDM_ODT_ON);
	}else{
		cmd_IDM_ODT(hWnd, IDM_ODT_OFF);
	}
	return ret;
}

int video_get_record_status(void)
{
	return video_rec_state;
}

static int fwk_video_queue;
static int fwk_wifi_queue;

int video_post_msg(int hWnd, int iMsg, WPARAM wParam, LPARAM lParam)
{
  int ret;
  ipc_msg msg;

  memset(&msg, 0, sizeof(msg));
  msg.msgid = iMsg;
  msg.wparam = wParam;
  msg.lparam = lParam;
  ret = fwk_send_message_ext(FWK_MOD_VIDEO, FWK_MOD_VIDEO, FWK_MSG_COMMON_IPC_MSG, &msg, sizeof(msg));
  if (ret < 0)
    printf("video_post_msg %d failed\n", iMsg);
  return ret;
}

#ifndef DEF_RK_ENABLE_UI
int send_msg_to_ui(int iMsg, WPARAM wParam, LPARAM lParam)
{
#if 0
	ipc_msg msg;
	msg.msgid = iMsg;
	msg.wparam = wParam;
	msg.lparam = lParam;
	return fwk_send_message_ext(FWK_MOD_VIDEO, FWK_MOD_VIEW, FWK_MSG_COMMON_IPC_MSG, &msg, sizeof(msg));
#endif
}
#endif

int send_msg_ex_to_ui(int iMsg, WPARAM wParam, LPARAM lParam, char *buf, int buf_len)
{
  int len = sizeof(ipc_msg) + buf_len;

  if (!buf || buf_len < 0) {
    return -1;
  }

  ipc_msg *msg = (ipc_msg *)malloc(len);

  msg->msgid = iMsg;
  msg->wparam = wParam;
  msg->lparam = lParam;
  msg->buf_len = buf_len;
  memcpy(msg->buf, buf, buf_len);

  fwk_send_message_ext(FWK_MOD_VIDEO, FWK_MOD_VIEW, FWK_MSG_COMMON_IPC_MSG, msg, len);

  free(msg);  
  return 0;
}

static void *main_loop(void* p_arg)
{
  FWK_MSG *msg = NULL;
  int ret = -1;
  ipc_msg *ipc_req;

  /*must add signal process, main loop exit, to do...*/
  while (1)
  {
    msg = fwk_get_message(fwk_video_queue, MSG_GET_WAIT_ROREVER, &ret);		//lyq gsensor data signal message to this
    if ((ret < 0) ||(msg == NULL)) {
    	fwk_debug("get message NULL\n");
    	continue;
    } else {
    	switch(msg->msg_id) {
    		case FWK_MSG_COMMON_IPC_MSG:
    			ipc_req = (ipc_msg*)msg->para;
			main_msg_proc(0, ipc_req->msgid, ipc_req->wparam, ipc_req->lparam);	// to do somethings
			if(ipc_req->msgid == MSG_WIFI_COMMAND) {
				printf("%s: MSG_WIFI_COMMAND\n",__FUNCTION__);
				handle_network_message(ipc_req->buf);
			}
    			break;
    		default:
    			fwk_debug("get err msgid %d\n", msg->msg_id);
    			break;
    	}
    }
    fwk_free_message(&msg);
  }

  return NULL;
}

static int create_message_queue(void)
{
    fwk_video_queue = fwk_create_queue(FWK_MOD_VIDEO);
    if (fwk_video_queue == -1)
    {
        printf("fwk_msg unable to create queue for main \n");
        return -1;
    }
    printf("create_message_queue, fwk_video_queue  id %d\n",fwk_video_queue);
    fwk_wifi_queue = fwk_create_queue(FWK_MOD_WIFI);
    if (fwk_wifi_queue == -1)
    {
        printf("fwk_msg unable to create queue for wifi \n");
        return -1;
    }
    printf("create_message_queue, fwk_wifi_queue  id %d\n",fwk_wifi_queue);
    return 0;
}

void my_alarm_handler(union sigval v)
{
	video_post_msg(hMainWnd, MSG_VIDEO_TIMER, 0, 0);
}

static int create_timer()
{
	struct sigevent evp;
	struct itimerspec ts;
	int ret;

	memset(&evp, 0, sizeof(struct sigevent));
	evp.sigev_value.sival_int = 111;
	evp.sigev_notify = SIGEV_THREAD;
	evp.sigev_notify_function = my_alarm_handler;

	ret = timer_create(CLOCK_REALTIME, &evp, &g_timer);
	if (ret)
		perror("timer_create");

	ts.it_interval.tv_sec = 1;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 1;
	ts.it_value.tv_nsec = 0;

	ret = timer_settime(g_timer, 0, &ts, NULL);
	if (ret)
		perror("timer_settime");
}

static int delete_timer(void)
{
	timer_delete(g_timer);
}
static int loadres(void)
{
	int i;
	char img[128];
	char respath[] = "/usr/local/share/minigui/res/images/";

	char watermark_img[7][30] = {"watermark.bmp", "watermark_240p.bmp", "watermark_360p.bmp", "watermark_480p.bmp",
		"watermark_720p.bmp", "watermark_1080p.bmp", "watermark_1440p.bmp"};
	/* load watermark bmp */
	for (i = 0; i < ARRAY_SIZE(watermark_bmap); i++) {
		sprintf(img, "%s%s", respath, watermark_img[i]);
		if (load_bitmap(&watermark_bmap[i], img)) {
			printf("load watermark bmp error, i = %d\n", i);
		}
	}
}

static void unloadres(void) {
	int i;
	/* unload watermark bmp */
	for (i = 0; i < ARRAY_SIZE(watermark_bmap); i++)
		unload_bitmap(&watermark_bmap[i]);
}

static void sig_pipe_handler(int sig)
{
  printf("SIGPIPE\n");
  return;
}

static void sig_int_handler(int sig)
{
  printf("exit with SIGINT\n");
  exit(0);
}

int main(int argc, const char *argv[])
{
  char collision_level = 0;
  char parkingrec = 0;
  pthread_t tid;

#ifdef USE_WATCHDOG
  char pathname[] = "/dev/watchdog";

  printf ("use watchdog\n");
  fd_wtd = open(pathname, O_WRONLY);
  if (fd_wtd == -1)
  	printf ("/dev/watchdog open error\n");
  else {
  	ioctl(fd_wtd, WDIOC_KEEPALIVE, 0);
  }
#endif
  printf("camera run\n");

  signal(SIGPIPE, sig_pipe_handler);
  signal(SIGINT, sig_int_handler);

#ifdef DEF_RK_ENABLE_UI
  ui_init(argc, argv);
#else
#if HAVE_DISPLAY 
  fb_init();
#endif
#endif
  /* Must initialize libcurl before any threads are started */
  curl_global_init(CURL_GLOBAL_ALL);
  fwk_msg_init();
  create_message_queue();
  create_timer();
  if(pthread_create(&tid, NULL, main_loop, (void *)NULL)) {
  	printf("Create run_usb_disconnect thread failure\n");
  	return -1;
  }

  video_record_init_lock(); //must before uevent init

  parameter_init();

  screenoff_time = parameter_get_screenoff_time();
#ifdef RK_DEF_AUDIO
  if (0 != audio_dev_init()) {
  	printf("audio_dev_init failed\n");
  }
#endif

#if defined(LED_ENABLE)
  if(!init_led_device("/sys/class/leds")) {
  	ipc_led_status(LED_STATUS_APP_STARTING);
  } else {
  	printf("init_led_device failed\n");
  }
#endif
    if (loadres()) {
        printf("loadres fail\n");
    }

  audio_recoder_init() ;
  pthread_mutex_init(&set_mutex, NULL);

  gsensor_init();

#ifdef NTP_SYNC_REQUEST /* ntp */
  ntp_init();
  if(1/*ntp_enable*/)
  	ntp_start(&ntp_servers);
#endif

  thermal_init();
  gsensor_use_interrupt(GSENSOR_INT2, GSENSOR_INT_STOP);

  /* register collision get data function */
  collision_level = parameter_get_collision_level();
  collision_init();
  if (collision_level != 0)
  	collision_register();
  collision_regeventcallback(collision_event_callback);

  /* register parkingrec get data function */
  parkingrec = parameter_get_leavecarrec();
  parking_init();
  if (parkingrec != 0)
  	parking_register();
  parking_regeventcallback(parking_event_callback);

#ifdef RK_DEF_SDCARD
  sd_reg_event_callback(sd_event_callback);
#endif
  usb_reg_event_callback(usb_event_callback);
  batt_reg_event_callback(batt_event_callback);
  poweroff_reg_callback(poweroff_callback);
  uevent_monitor_run();
#if 1
  kbd_init();
  kbd_register(key_event_callback);
#endif
#ifdef RK_DEF_SDCARD
  REC_RegEventCallback(rec_event_callback);
#endif
#ifdef ENABLE_ODT_DEBUG
  MD_RegEventCallback(rec_event_callback);
#endif
  MD_RegEventCallback(OdtWrapper_getRecEventCb());
  USER_RegEventCallback(user_event_callback);
  hdmi_reg_event_callback(hdmi_event_callback);
  mouse_reg_event_callback(mouse_event_callback);
  camera_reg_event_callback(camere_event_callback);

  /*
   * If file size have been changed, need call the callball to nofity fs_manage
   * to change storage policy. Such as recordtime, resolution,
   * bit_rate_per_pixel, all they will effect file size.
   */
#ifdef RK_DEF_SDCARD
  storage_setting_reg_event_callback(storage_setting_callback);
  storage_setting_event_callback(0, NULL, NULL);

  video_record_setaudio(parameter_get_video_audio());
  initrec(hMainWnd);

  pm_register("record", SLEEP_IN_ULOW,
                   record_get_state, NULL,
                   record_slp_in, NULL,
                   record_slp_out, NULL);
#endif

#ifdef RK_DEF_WIFI
  if (parameter_get_wifi_en() == 1)
  	wifi_management_start();
#endif

client_socket_init() ;
  FlagForUI_ca.adasflagtooff = 0;
  FlagForUI_ca.formatflag = 0;
  FlagForUI_ca.setmode_ui = 0;
  FlagForUI_ca.sdcard_ui = sdcard;
  FlagForUI_ca.video_rec_state_ui = video_rec_state;
  rk_protocol_gui_init(hMainWnd, video_rec_state);

  rk_gyrosensor_init();

#if defined(LED_ENABLE)
  ipc_led_status(LED_STATUS_APP_IDLE);
#endif

#ifdef MSG_FWK
  rk_fwk_glue_init();
  rk_fwk_controller_init();
#endif

#ifdef PROTOCOL_GB28181
  protocol_rk_gb28181_init();
#endif

#ifdef PROTOCOL_IOTC
  protocol_rk_iotc_init();
#endif

#ifdef PROTOCOL_ONVIF
  protocol_rk_onvif_init();
#endif


#ifdef RTSP_SERVER
  start_rtsp_server(NULL);
#endif

#if USE_USB_WEBCAM
  uvc_gadget_pthread_exit();
  uvc_gadget_pthread_init();
#endif

#ifdef DEF_RK_ENABLE_UI
  ui_create_main_win();
#else
  pthread_join(tid, NULL);
#endif

#ifdef USE_WATCHDOG
  //printf ("close watchdog\n");
  //sleep(2);
  //if (fd_wtd != -1) {
  	//write(fd_wtd, "V", 1);
  	//close(fd_wtd);
  //}
#endif

#ifdef PROTOCOL_ONVIF
  protocol_rk_onvif_destroy();
#endif


#ifdef PROTOCOL_IOTC
  protocol_rk_iotc_destroy();
#endif

#ifdef PROTOCOL_GB28181
  protocol_rk_gb28181_destroy();
#endif

#ifdef MSG_FWK
  rk_fwk_controller_destroy();
  rk_fwk_glue_destroy();
#endif

  rk_gyrosensor_deinit();

#ifdef RK_DEF_WIFI
  wifi_management_stop();
#endif
#ifdef RK_DEF_SDCARD
  deinitrec(hMainWnd);
  video_record_deinit();
#endif
#ifndef RK_DEF_SO
  videoplay_deinit();
#endif
#ifdef RK_DEF_SDCARD
  fs_manage_deinit();
#endif

  collision_unregister();

  parking_unregister();
  parkingrec = parameter_get_leavecarrec();
  if (parkingrec != 0) {
  	gsensor_enable(1);
  	gsensor_use_interrupt(GSENSOR_INT2, GSENSOR_INT_START);
  }
  gsensor_release();
  unloadres();
  delete_timer();

#ifdef RK_DEF_AUDIO
  audio_dev_deinit();
#endif
  video_record_destroy_lock(); //must after uevent deinit
  curl_global_cleanup();
  printf("camera exit\n");

#ifdef DEF_RK_ENABLE_UI
  ui_deinit(0);
#endif
  return 0;
}
