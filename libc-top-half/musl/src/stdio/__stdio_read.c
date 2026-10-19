#ifdef __wasilibc_unmodified_upstream // WASI has no syscall
#else
#include <unistd.h>
#endif
#include "stdio_impl.h"
#include <sys/uio.h>

size_t __stdio_read(FILE *f, unsigned char *buf, size_t len)
{
	// 標準入力(fd: 0)から実際にデータを読み込む直前に stdout をフラッシュする
	// ロックによるデッドロックを防ぐため fflush_unlocked を使用
	if ((f == stdin || f->fd == 0) && stdout)
	{
		fflush_unlocked(stdout);
	}

	struct iovec iov[2] = {
		{ .iov_base = buf, .iov_len = len - !!f->buf_size },
		{ .iov_base = f->buf, .iov_len = f->buf_size }
	};
	ssize_t cnt;

#ifdef __wasilibc_unmodified_upstream // WASI has no syscall
	cnt = iov[0].iov_len ? syscall(SYS_readv, f->fd, iov, 2)
		: syscall(SYS_read, f->fd, iov[1].iov_base, iov[1].iov_len);
#else
	cnt = iov[0].iov_len ? readv(f->fd, iov, 2)
		: read(f->fd, iov[1].iov_base, iov[1].iov_len);
#endif
	if (cnt <= 0) {
		f->flags |= cnt ? F_ERR : F_EOF;
		return 0;
	}
	if (cnt <= iov[0].iov_len) return cnt;
	cnt -= iov[0].iov_len;
	f->rpos = f->buf;
	f->rend = f->buf + cnt;
	if (f->buf_size) buf[len-1] = *f->rpos++;
	return len;
}
