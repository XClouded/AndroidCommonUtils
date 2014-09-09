#include "common.h"

#define LOG_TAG "termExec"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <jni.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <linux/kd.h>
#include <linux/fb.h>
#include <stddef.h>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <asm/page.h>

#include "termExec.h"
#include "turbojpeg.h"

typedef unsigned char uint8;
#define MEMACCESS(base) "%%nacl:(%%r15,%q" #base ")"

// ====================================================
// ====================================================
// ndk-build 要用这个命令速度才能快起来：（用到Neon协处理器）
//		ndk-build APP_ABI=armeabi-v7a LOCAL_ARM_MODE=arm LOCAL_ARM_NEON=true ARCH_ARM_HAVE_NEON=true
// ====================================================
// ====================================================

void ScaleRowDown2_NEON(const uint8* src_ptr, uint8* dst, int dst_width) {
  __asm__ volatile (
    ".p2align   2                              \n"
	"1:                                          \n"
    // load even pixels into q0, odd into q1
    // 一次性 load 两字节到 q0 q1
    "vld2.8     {q0, q1}, [%0]!                \n"
    // dst_width -= 16;
    "subs       %2, %2, #16                    \n"  // 2 bytes processed per loop
    // 把 q1 保存到 dst
    "vst1.8     {q1}, [%1]!                    \n"  // store odd pixels
    "bgt        1b                             \n"
  : "+r"(src_ptr),          // %0
    "+r"(dst),              // %1
    "+r"(dst_width)         // %2
  :
  : "q0", "q1"              // Clobber List
  );
}

static void socket_Test() {

}

static int android_os_Exec_test(JNIEnv *env, jobject clazz, jbyteArray ba) {
	/*
	 * 像素偏移量计算公式
	 * location=(x+vinfo.xoffset)*(vinfo.bits_per_pixel/8)+
	 *	                    (y+vinfo.yoffset)*finfo.line_length;
	 */
	int fd, ret, byte_per_frame;
	unsigned long	jpegSize = 0;
	// 映射 /dev/graphics/fb0 内存地址
	unsigned char	*framebuffer_memory;
	// 转换成 jpeg 数据后的地址
	unsigned char* jpeg_data = NULL;

	static struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	// turbo-jpeg 实例
	tjhandle	 handle;

	fd = open("/dev/graphics/fb0", O_RDONLY);

	if(fd < 0)
	{
		LOGD("======Cannot open /dev/graphics/fb0!");
		return -1;
	}

	// variable info 可变信息
	ret = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
	if(ret < 0 )
	{
		LOGD("======Cannot get variable screen information.");
		close(fd);
		return -1;
	}

	// 这两个是显示在显示屏上时候的分辨率
	LOGD("====== xres : %d",  vinfo.xres);
	LOGD("====== yres : %d",  vinfo.yres);
	// 这两个是显存缓存的分辨率 如果显存缓存了两个屏幕的时候
	// yres_virtula 应该等于 yres * 2
	// 而 xres_virtual 就应该 == xres
	LOGD("====== xres_virtual : %d",  vinfo.xres_virtual);
	LOGD("====== yres_virtual : %d",  vinfo.yres_virtual);

	/* offset from virtual to visible */
	// 显存可能缓存了多个屏幕，哪到底哪个屏幕才是显示屏应该显示的内容呢
	// 这就是由下面这两个offset来决定了
	LOGD("====== xoffset : %d",  vinfo.xoffset);
	LOGD("====== yoffset : %d",  vinfo.yoffset);

	LOGD("====== bits_per_pixel : %d",  vinfo.bits_per_pixel);

	// 下面这一段是每个像素点的格式
	LOGD("====== fb_bitfield red.offset : %d",  vinfo.red.offset);
	LOGD("====== fb_bitfield red.length : %d",  vinfo.red.length);
	// 如果 == 0，指的是数据的最高有效位在最左边 也就是Big endian
	LOGD("====== fb_bitfield red.msb_right : %d",  vinfo.red.msb_right);
	LOGD("====== fb_bitfield green.offset : %d",  vinfo.green.offset);
	LOGD("====== fb_bitfield green.length : %d",  vinfo.green.length);
	LOGD("====== fb_bitfield green.msb_right : %d",  vinfo.green.msb_right);
	LOGD("====== fb_bitfield blue.offset : %d",  vinfo.blue.offset);
	LOGD("====== fb_bitfield blue.length : %d",  vinfo.blue.length);
	LOGD("====== fb_bitfield blue.msb_right : %d",  vinfo.blue.msb_right);
	LOGD("====== fb_bitfield transp.offset : %d",  vinfo.transp.offset);
	LOGD("====== fb_bitfield transp.length : %d",  vinfo.transp.length);
	LOGD("====== fb_bitfield transp.msb_right : %d",  vinfo.transp.msb_right);

	LOGD("====== height : %d",  vinfo.height);
	// width of picture in mm 毫米
	LOGD("====== width : %d",  vinfo.width);
	// UP 0
	// CW 1
	// UD 2
	// CCW 3
	/* angle we rotate counter clockwise */
	LOGD("====== rotate : %d",  vinfo.rotate);

	// fixed info 不变信息
	ret = ioctl(fd, FBIOGET_FSCREENINFO, &finfo);
	if(ret < 0 )
	{
		LOGD("Cannot get fixed screen information.");
		close(fd);
		return -1;
	}

	LOGD("====== smem_start : %lu",  finfo.smem_start);
	// Framebuffer设备的大小
	LOGD("====== smem_len : %d",  finfo.smem_len);
	// 一行的byte数目 除以 (bits_per_pixel/8) 就是一行的像素点的数目
	LOGD("====== line_length : %d",  finfo.line_length);

	//FB_TYPE_PACKED_PIXELS                0       /* Packed Pixels        */
	//FB_TYPE_PLANES                            1       /* Non interleaved planes */
	//FB_TYPE_INTERLEAVED_PLANES      2       /* Interleaved planes   */
	//FB_TYPE_TEXT                                3       /* Text/attributes      */
	//FB_TYPE_VGA_PLANES                    4       /* EGA/VGA planes       */
	//FB_TYPE_FOURCC                          5       /* Type identified by a V4L2 FOURCC */
	LOGD("====== type : %d",  finfo.type);
	// 每屏数据的字节数
	byte_per_frame = finfo.line_length * vinfo.yres;

	framebuffer_memory = (unsigned char*)mmap(
			0,
			byte_per_frame,
			PROT_READ,
			// 不要用MAP_PRIVATE
			MAP_SHARED,
			fd,
			// 假定xoffset为0
			finfo.line_length * vinfo.yoffset);

	if(framebuffer_memory == MAP_FAILED) {
		close(fd);
		LOGE("mmap for fb0 failed!");
		return -1;
	}

	handle = tjInitCompress();

	jpeg_data = (unsigned char *) malloc(byte_per_frame);

//	ScaleRowDown2_NEON(framebuffer_memory, jpeg_data, byte_per_frame * 8);

	tjCompress2(handle, framebuffer_memory,
			// 希望生成的jpeg图片的宽 源数据里头屏幕每行的字节数
			1280, finfo.line_length,
			// 希望生成的jpeg图片的高
			720, TJPF_BGRA,
			&jpeg_data, &jpegSize,
			TJSAMP_444, 50,
			TJFLAG_NOREALLOC);

	LOGD("Turbo-jpeg compress fb0 done!");

	// 释放资源
	munmap(framebuffer_memory, byte_per_frame);

	(*env)->SetByteArrayRegion(env,
			ba,
			0,
			jpegSize,
			(jbyte*)jpeg_data);

	free(jpeg_data);
	close(fd);
	tjDestroy(handle);

	return jpegSize;
}

static const char *classPathName = "com/a1w0n/standard/Jni/Exec";

/**
 * 写法：
 *     括号 () 里头的是参数 紧跟着的是返回值 比如 ： (I)I 就是参数是一个int 返回值也是一个int
 *	   返回值void 用 V 表示
 *
 *	   值得注意的一点是，当参数类型是引用数据类型时，其格式是“L包名；”其中包名中的“.” 换成“/”，
 *	   所以在上面的例子中(Ljava/lang/String;Ljava/lang/String;)V 表示 void Func(String,String)；
 *
 *	   数组则以”["开始,用两个字符表示
 *	   [B jbyteArra byte[]
 */
static JNINativeMethod method_table[] = { { "test", "([B)I",
		(void*) android_os_Exec_test } };

int init_Exec(JNIEnv *env) {
	if (!registerNativeMethods(env, classPathName, method_table,
			sizeof(method_table) / sizeof(method_table[0]))) {
		return JNI_FALSE;
	}

	return JNI_TRUE;
}
