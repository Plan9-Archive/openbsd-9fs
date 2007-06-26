/* o9fs_subr.c */
int o9fs_alloc_dirent(struct o9fsnode *, const char *, uint16_t namelen, struct o9fsdirent **);
int o9fs_alloc_node(struct o9fsnode **, enum vtype);
int o9fs_alloc_vp(struct mount *, struct o9fsnode *, struct vnode **, u_long);
int o9fs_create_file(struct vnode *, struct vnode **, struct vattr *, struct componentname *);
void o9fs_dir_attach(struct vnode *, struct o9fsdirent *);
struct o9fsdirent *o9fs_dir_lookup(struct o9fsnode *, struct componentname *);
void o9fs_disconnect(struct o9fsmount *);
void *o9fs_rpc(struct socket *, struct o9fsfcall *, struct o9fsfcall *);

/* o9fs_io.c */
int		o9fs_tcp_open(struct o9fsmount *);

/* o9fs_conv*.c */
u_int	o9fs_convM2S(u_char*, u_int, struct o9fsfcall*);
u_int	o9fs_convS2M(struct o9fsfcall*, u_char*, u_int);
u_int	o9fs_sizeS2M(struct o9fsfcall*);
int		o9fs_statcheck(u_char *abuf, u_int nbuf);

extern int (**o9fs_vnodeop_p)(void *);

