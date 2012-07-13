#include <sys/param.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/queue.h>

#include "o9fs.h"
#include "o9fs_extern.h"

enum{
	Debug = 0,
};

void
o9fs_clunk(struct o9fs *fs, struct o9fid *f)
{
	DIN();

	if (f == NULL)
		panic("o9fs_clunk: nil fid");

	O9FS_PBIT32(fs->outbuf, 11);
	O9FS_PBIT8(fs->outbuf + Offtype, O9FS_TCLUNK);
	O9FS_PBIT16(fs->outbuf + Offtag, 0);
	O9FS_PBIT32(fs->outbuf + Minhd, f->fid);
	
	o9fs_mio(fs, 11);
	DRET();
}

struct o9fid *
o9fs_walk(struct o9fs *fs, struct o9fid *fid, struct o9fid *newfid, char *name)
{
	long n;
	u_char *p;
	int nwname, nwqid;
	struct o9fsqid *nqid;
	DIN();

	if (fid == NULL) {
		DRET();
		return NULL;
	}

	p = fs->outbuf;
	O9FS_PBIT8(p + Offtype, O9FS_TWALK);
	O9FS_PBIT16(p + Offtag, 0);
	O9FS_PBIT32(p + Minhd, fid->fid);

	if (newfid == NULL) {
		DBG("cloning fid %d\n", fid->fid);
		newfid = o9fs_xgetfid(fs);
		newfid->mode = fid->mode;
		newfid->qid = fid->qid;
		newfid->offset = fid->offset;
		newfid->parent = fid->parent;
		newfid->ref = fid->ref;
		nwname = 0;
		p += Minhd + 4 + 4 + 2;		/* Advance after nwname, which will be filled later */
	}

	if (name != NULL) {
		p = putstring(fs->outbuf + Minhd + 4 + 4 + 2, name);
		nwname = 1;
	}

	DBG("fid %p %d newfid %p %d\n", fid, fid->fid, newfid, newfid->fid);

	O9FS_PBIT32(fs->outbuf + Minhd + 4, newfid->fid);
	O9FS_PBIT16(fs->outbuf + Minhd + 4 + 4, nwname);

	n = p - fs->outbuf;
	O9FS_PBIT32(fs->outbuf, n);
	n = o9fs_mio(fs, n);
	if (n <= 0) {
		o9fs_xputfid(fs, newfid);
		DRET();
		return NULL;
	}

	nwqid = O9FS_GBIT16(fs->inbuf + Minhd);
	if (nwqid < nwname) {
		printf("nwqid < nwname\n");
		DRET();
		return NULL;
	}

	if (nwname > 0) {
		newfid->qid.type = O9FS_GBIT8(fs->inbuf + Minhd + 2);
		newfid->qid.vers = O9FS_GBIT32(fs->inbuf + Minhd + 2 + 1);
		newfid->qid.path = O9FS_GBIT64(fs->inbuf + Minhd + 2 + 1 + 4);
	}

	DRET();
	return newfid;
}

struct o9fsstat *
o9fs_stat(struct o9fs *fs, struct o9fid *fid)
{
	long n, nstat;
	struct o9fsstat *stat;
	DIN();

	if (fid == NULL) {
		DRET();
		return NULL;
	}

	O9FS_PBIT32(fs->outbuf, Minhd + 4);
	O9FS_PBIT8(fs->outbuf + Offtype, O9FS_TSTAT);
	O9FS_PBIT16(fs->outbuf + Offtag, 0);
	O9FS_PBIT32(fs->outbuf + Minhd, fid->fid);
	n = o9fs_mio(fs, Minhd + 4);
	if (n <= 0) {
		DRET();
		return NULL;
	}

	n = O9FS_GBIT16(fs->inbuf + Minhd + 2);
	stat = malloc(n, M_O9FS, M_WAITOK);

	stat->type = O9FS_GBIT16(fs->inbuf + Minhd + 2 + 2);
	stat->dev = O9FS_GBIT32(fs->inbuf + Minhd + 2 + 2 + 2);
	stat->qid.type = O9FS_GBIT8(fs->inbuf + Minhd + 2 + 2 + 2 + 4);
	stat->qid.vers = O9FS_GBIT32(fs->inbuf + Minhd + 2 + 2 + 2 + 4 + 1);
	stat->qid.path = O9FS_GBIT64(fs->inbuf + Minhd + 2 + 2 + 2 + 4 + 1 + 4);
	stat->mode = O9FS_GBIT32(fs->inbuf + Minhd + 2 + 2 + 2 + 4 + 1 + 4 + 8);
	stat->atime = O9FS_GBIT32(fs->inbuf + Minhd + 2 + 2 + 2 + 4 + 1 + 4 + 8 + 4);
	stat->mtime = O9FS_GBIT32(fs->inbuf + Minhd + 2 + 2 + 2 + 4 + 1 + 4 + 8 + 4 + 4);
	stat->length = O9FS_GBIT64(fs->inbuf + Minhd + 2 + 2 + 2 + 4 + 1 + 4 + 8 + 4 + 4 + 4);

	/* For now the other fields are not used, so we don't bother parsing them */

	DRET();
	return stat;
}
	
uint32_t
o9fs_rdwr2(struct o9fs *fs, struct o9fid *f, uint8_t type, uint32_t len, uint64_t off)
{
	u_char *p;
	long n;
	DIN();

	if (f == NULL) {
		DRET();
		return -1;
	}

	p = fs->outbuf;
	O9FS_PBIT8(p + Offtype, type);
	O9FS_PBIT16(p + Offtag, 0);
	O9FS_PBIT32(p + Minhd, f->fid);
	O9FS_PBIT64(p + Minhd + 4, off);
	if (f->iounit > 0 && len > f->iounit)
		len = f->iounit;
	if (len > fs->msize)
		len = fs->msize - O9FS_IOHDRSZ;

	O9FS_PBIT32(p + Minhd + 4 + 8, len);
	p += Minhd + 4 + 8 + 4;
	if (type == O9FS_TWRITE)
		p += len;
	n = p - fs->outbuf;

	O9FS_PBIT32(fs->outbuf, n);
	n = o9fs_mio(fs, n);
	if (n <= 0) {
		DRET();
		return -1;
	}

	n = O9FS_GBIT32(fs->inbuf + Minhd);
	DRET();
	return n;
}
	
int
o9fs_opencreate2(struct o9fs *fs, struct o9fid *fid, uint8_t type, uint8_t mode, uint32_t perm, char *name)
{
	long n;
	u_char *p;
	uint8_t omode;
	DIN();

	if (fid == NULL) {
		DRET();
		return -1;
	}

	p = fs->outbuf;
	O9FS_PBIT8(p + Offtype, type);
	O9FS_PBIT16(p + Offtag, 0);
	O9FS_PBIT32(p + Minhd, fid->fid);
	p += Minhd + 4;

	if (type == O9FS_TCREATE) {
		if (name == NULL) {
			DRET();
			return -1;
		}
		p = putstring(p, name);
		O9FS_PBIT32(p, perm);
		p += 4;
	}
	omode = o9fs_uflags2omode(mode);
	O9FS_PBIT8(p, omode);
	n = p + 1 - fs->outbuf;

	O9FS_PBIT32(fs->outbuf, n);
	n = o9fs_mio(fs, n);
	if (n <= 0) {
		DRET();
		return -1;
	}

	fid->qid.type = O9FS_GBIT8(fs->inbuf + Minhd);
	fid->qid.vers = O9FS_GBIT32(fs->inbuf + Minhd + 1);
	fid->qid.path = O9FS_GBIT64(fs->inbuf + Minhd + 1 + 4);
	fid->iounit = O9FS_GBIT32(fs->inbuf + Minhd + 1 + 4 + 8);
	fid->mode = omode;
	return 0;
}
