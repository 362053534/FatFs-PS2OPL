/* Exercise the real implementation with injected disk/mutex failures. */
#include <assert.h>
#include <stdio.h>
#include "../source/ff.c"

static DSTATUS status;
static DRESULT write_result, sync_result;
static int held, takes, fail_take;

DSTATUS disk_status(BYTE drive) { (void)drive; return status; }
DRESULT disk_read(BYTE d, BYTE *b, LBA_t s, UINT n)
{ (void)d; (void)b; (void)s; (void)n; return RES_ERROR; }
DRESULT disk_write(BYTE d, const BYTE *b, LBA_t s, UINT n)
{ (void)d; (void)b; (void)s; (void)n; return write_result; }
DRESULT disk_ioctl(BYTE d, BYTE cmd, void *b)
{ (void)d; (void)b; assert(cmd == CTRL_SYNC); return sync_result; }
DWORD get_fattime(void) { return 0; }
int ff_mutex_take(int vol)
{
    (void)vol;
    if (++takes == fail_take) return 0;
    assert(!held);
    held = 1;
    return 1;
}
void ff_mutex_give(int vol) { (void)vol; assert(held); held = 0; }

static FATFS fs;
static FIL file;
static DIR dir;
static void reset(void)
{
    assert(!held);
    memset(&fs, 0, sizeof fs);
    memset(&file, 0, sizeof file);
    memset(&dir, 0, sizeof dir);
    fs.fs_type = FS_FAT16;
    fs.id = 1;
    fs.ssize = 512;
    fs.winsect = 10;
    file.obj.fs = dir.obj.fs = &fs;
    file.obj.id = dir.obj.id = fs.id;
    file.dir_sect = fs.winsect;
    file.dir_ptr = fs.win;
    status = 0;
    write_result = sync_result = RES_OK;
    takes = fail_take = 0;
}
static void close_file(FRESULT expected)
{
    assert(f_close(&file) == expected);
    assert(file.obj.fs == NULL && !held);
    assert(f_close(&file) == FR_INVALID_OBJECT);
}
static void close_dir(FRESULT expected)
{
    assert(f_closedir(&dir) == expected);
    assert(dir.obj.fs == NULL && !held);
    assert(f_closedir(&dir) == FR_INVALID_OBJECT);
}
int main(void)
{
    int i;
    assert(f_close(NULL) == FR_INVALID_OBJECT);
    assert(f_closedir(NULL) == FR_INVALID_OBJECT);
    reset(); close_file(FR_OK); close_dir(FR_OK);
    reset(); file.flag = FA_MODIFIED; close_file(FR_OK);
    reset(); file.flag = FA_MODIFIED; write_result = RES_ERROR;
    close_file(FR_DISK_ERR);
    reset(); file.flag = FA_MODIFIED; sync_result = RES_ERROR;
    close_file(FR_DISK_ERR);
    reset(); fail_take = 1; close_file(FR_TIMEOUT);
    reset(); fail_take = 2; close_file(FR_TIMEOUT); /* validate after sync */
    reset(); fail_take = 1; close_dir(FR_TIMEOUT);
    reset(); ++fs.id; close_file(FR_INVALID_OBJECT); close_dir(FR_INVALID_OBJECT);
    reset(); fs.fs_type = 0; close_file(FR_INVALID_OBJECT); close_dir(FR_INVALID_OBJECT);
    /* Reuse the same slots beyond both the driver's 128-file/16-dir limits. */
    for (i = 0; i < 1024; ++i) {
        reset(); status = STA_NOINIT;
        close_file(FR_INVALID_OBJECT); close_dir(FR_INVALID_OBJECT);
    }
    puts("close regression tests passed");
    return 0;
}
