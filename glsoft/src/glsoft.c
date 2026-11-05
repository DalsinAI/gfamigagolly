/*
 * GLSoft: the part of OpenGL 1.1 Golly's pattern view draws with, in
 * software, into a 32-bit 0xAARRGGBB picture (see GL/gl.h).
 *
 * Everything Golly draws is axis aligned: each rectangle comes as a
 * triangle strip or fan of 4 vertices, or as 2 triangles (6 vertices) of
 * GL_TRIANGLES, and its grid as horizontal and vertical lines. So a
 * rectangle is filled as a box (the box round its vertices, in picture
 * pixels) and textured by stepping its texture box across it in 16.16
 * fixed point, nearest texel; one texel a pixel (Golly's usual case) is a
 * straight run. Lines that are not axis aligned are stepped (DDA).
 *
 * The state that goes with a context (enables, colour, blend, matrices,
 * viewport, arrays) is per context; textures are one table shared by all,
 * as Golly names its textures in globals.
 *
 * Copyright (c) 2026 AmiCradle Team
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "GL/gl.h"
#include "glsoft.h"

typedef unsigned int u32;

typedef struct { float sx, sy, tx, ty; } Mat;   /* x' = sx*x + tx, y' = sy*y + ty */

#define STACK 16

struct glsoft_ctx {
    int tex2d, blend;
    unsigned char cr, cg, cb, ca;
    float clr, clg, clb, cla;
    GLenum mode;
    Mat mv[STACK], pj[STACK];
    int mvn, pjn;
    int vx, vy, vw, vh;
    const void *vp; GLenum vtype; int vsize, vstride;
    const void *tp; GLenum ttype; int tsize, tstride;
    GLuint bound;
    GLenum err;
};

typedef struct {
    int used;
    int w, h;
    u32 *px;        /* 0xAARRGGBB */
} Tex;

static struct glsoft_ctx defctx;
static struct glsoft_ctx *C = &defctx;
static int defctx_ready;

static Tex *texs;
static int ntexs;

static u32 *fb;
static int fbw, fbh, fbpitch;

static void ctx_init(struct glsoft_ctx *c)
{
    memset(c, 0, sizeof *c);
    c->cr = c->cg = c->cb = c->ca = 255;
    c->mode = GL_MODELVIEW;
    c->mv[0].sx = c->mv[0].sy = 1;
    c->pj[0].sx = c->pj[0].sy = 1;
    c->vtype = c->ttype = GL_FLOAT;
    c->vsize = c->tsize = 2;
}

static void ensure_def(void)
{
    if (!defctx_ready) { ctx_init(&defctx); defctx_ready = 1; }
}

struct glsoft_ctx *glsoftCreateContext(void)
{
    struct glsoft_ctx *c = (struct glsoft_ctx *)malloc(sizeof *c);
    if (c) ctx_init(c);
    return c;
}

void glsoftDestroyContext(struct glsoft_ctx *c)
{
    if (!c) return;
    if (C == c) { ensure_def(); C = &defctx; }
    free(c);
}

void glsoftMakeCurrent(struct glsoft_ctx *c)
{
    ensure_def();
    C = c ? c : &defctx;
}

void glsoftSetTarget(unsigned int *pixels, int width, int height, int pitch)
{
    ensure_def();
    fb = pixels; fbw = width; fbh = height; fbpitch = pitch;
    if (!fb) fbw = fbh = 0;
}

/* ---- state ------------------------------------------------------------ */

void glEnable(GLenum cap)
{
    ensure_def();
    if (cap == GL_TEXTURE_2D) C->tex2d = 1;
    else if (cap == GL_BLEND) C->blend = 1;
}

void glDisable(GLenum cap)
{
    ensure_def();
    if (cap == GL_TEXTURE_2D) C->tex2d = 0;
    else if (cap == GL_BLEND) C->blend = 0;
}

GLboolean glIsEnabled(GLenum cap)
{
    ensure_def();
    if (cap == GL_TEXTURE_2D) return (GLboolean)C->tex2d;
    if (cap == GL_BLEND) return (GLboolean)C->blend;
    if (cap == GL_VERTEX_ARRAY || cap == GL_TEXTURE_COORD_ARRAY) return GL_TRUE;
    return GL_FALSE;
}

/* the arrays are always read: Golly turns them on once and never draws
   without them */
void glEnableClientState(GLenum array) { (void)array; }
void glDisableClientState(GLenum array) { (void)array; }

/* the one blend there is: source alpha, one minus source alpha */
void glBlendFunc(GLenum s, GLenum d) { (void)s; (void)d; }

void glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
    ensure_def();
    C->clr = r; C->clg = g; C->clb = b; C->cla = a;
}

static unsigned char to8(float f)
{
    if (f <= 0) return 0;
    if (f >= 1) return 255;
    return (unsigned char)(f * 255.0f + 0.5f);
}

void glClear(GLbitfield mask)
{
    ensure_def();
    if (!(mask & GL_COLOR_BUFFER_BIT) || !fb) return;
    u32 c = 0xff000000u | ((u32)to8(C->clr) << 16) | ((u32)to8(C->clg) << 8) | to8(C->clb);
    for (int y = 0; y < fbh; y++) {
        u32 *p = fb + (long)y * fbpitch;
        for (int x = 0; x < fbw; x++) p[x] = c;
    }
}

void glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
    ensure_def();
    C->cr = r; C->cg = g; C->cb = b; C->ca = a;
}

void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    glColor4ub(to8(r), to8(g), to8(b), to8(a));
}

void glLineWidth(GLfloat w) { (void)w; }
void glPixelStorei(GLenum p, GLint v) { (void)p; (void)v; }
void glScissor(GLint x, GLint y, GLsizei w, GLsizei h) { (void)x; (void)y; (void)w; (void)h; }
void glFlush(void) {}
void glFinish(void) {}

GLenum glGetError(void)
{
    ensure_def();
    GLenum e = C->err;
    C->err = GL_NO_ERROR;
    return e;
}

const GLubyte *glGetString(GLenum name)
{
    switch (name) {
    case GL_VENDOR: return (const GLubyte *)"AmiCradle Team";
    case GL_RENDERER: return (const GLubyte *)"GLSoft (68040)";
    /* 2.1: Golly then takes textures of any size, with no padding */
    case GL_VERSION: return (const GLubyte *)"2.1 GLSoft";
    case GL_EXTENSIONS: return (const GLubyte *)"";
    }
    return NULL;
}

#define MAX_TEX 4096

void glGetIntegerv(GLenum pname, GLint *params)
{
    ensure_def();
    if (!params) return;
    switch (pname) {
    case GL_MAX_TEXTURE_SIZE: params[0] = MAX_TEX; break;
    case GL_VIEWPORT: params[0] = C->vx; params[1] = C->vy; params[2] = C->vw; params[3] = C->vh; break;
    default: params[0] = 0; break;
    }
}

/* ---- matrices --------------------------------------------------------- */

static Mat *cur(void)
{
    return C->mode == GL_PROJECTION ? &C->pj[C->pjn] : &C->mv[C->mvn];
}

void glMatrixMode(GLenum mode) { ensure_def(); C->mode = mode; }

void glLoadIdentity(void)
{
    ensure_def();
    Mat *m = cur();
    m->sx = m->sy = 1; m->tx = m->ty = 0;
}

void glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f)
{
    (void)n; (void)f;
    ensure_def();
    if (r == l || t == b) { C->err = GL_INVALID_VALUE; return; }
    Mat *m = cur();
    float osx = (float)(2.0 / (r - l)), otx = (float)(-(r + l) / (r - l));
    float osy = (float)(2.0 / (t - b)), oty = (float)(-(t + b) / (t - b));
    m->tx += m->sx * otx; m->sx *= osx;
    m->ty += m->sy * oty; m->sy *= osy;
}

void glViewport(GLint x, GLint y, GLsizei w, GLsizei h)
{
    ensure_def();
    C->vx = x; C->vy = y; C->vw = w; C->vh = h;
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    (void)z;
    ensure_def();
    Mat *m = cur();
    m->tx += m->sx * x;
    m->ty += m->sy * y;
}

void glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    (void)z;
    ensure_def();
    Mat *m = cur();
    m->sx *= x; m->sy *= y;
}

void glPushMatrix(void)
{
    ensure_def();
    if (C->mode == GL_PROJECTION) {
        if (C->pjn + 1 >= STACK) { C->err = GL_INVALID_OPERATION; return; }
        C->pj[C->pjn + 1] = C->pj[C->pjn]; C->pjn++;
    } else {
        if (C->mvn + 1 >= STACK) { C->err = GL_INVALID_OPERATION; return; }
        C->mv[C->mvn + 1] = C->mv[C->mvn]; C->mvn++;
    }
}

void glPopMatrix(void)
{
    ensure_def();
    if (C->mode == GL_PROJECTION) { if (C->pjn > 0) C->pjn--; else C->err = GL_INVALID_OPERATION; }
    else { if (C->mvn > 0) C->mvn--; else C->err = GL_INVALID_OPERATION; }
}

/* object x, y to picture x, y: fx = ax*x + bx, fy = ay*y + by */
static void picture_map(float *ax, float *bx, float *ay, float *by)
{
    const Mat *m = &C->mv[C->mvn], *p = &C->pj[C->pjn];
    float sx = p->sx * m->sx, tx = p->sx * m->tx + p->tx;
    float sy = p->sy * m->sy, ty = p->sy * m->ty + p->ty;
    float hw = C->vw * 0.5f, hh = C->vh * 0.5f;
    /* window x = vx + (ndc + 1) * vw / 2; picture y = height - window y */
    *ax = sx * hw;
    *bx = C->vx + (tx + 1) * hw;
    *ay = -sy * hh;
    *by = fbh - (C->vy + (ty + 1) * hh);
}

/* ---- arrays ----------------------------------------------------------- */

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
    ensure_def();
    C->vsize = size; C->vtype = type; C->vstride = stride; C->vp = ptr;
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
    ensure_def();
    C->tsize = size; C->ttype = type; C->tstride = stride; C->tp = ptr;
}

static int type_size(GLenum t)
{
    switch (t) {
    case GL_SHORT: case GL_UNSIGNED_SHORT: return 2;
    case GL_INT: case GL_UNSIGNED_INT: case GL_FLOAT: return 4;
    case GL_DOUBLE: return 8;
    case GL_BYTE: case GL_UNSIGNED_BYTE: return 1;
    }
    return 4;
}

static void fetch2(const void *base, GLenum type, int size, int stride, int i, float *a, float *b)
{
    int es = type_size(type);
    const char *p = (const char *)base + (long)i * (stride ? stride : size * es);
    switch (type) {
    case GL_FLOAT: *a = ((const float *)p)[0]; *b = ((const float *)p)[1]; break;
    case GL_SHORT: *a = ((const short *)p)[0]; *b = ((const short *)p)[1]; break;
    case GL_INT: *a = (float)((const int *)p)[0]; *b = (float)((const int *)p)[1]; break;
    case GL_DOUBLE: *a = (float)((const double *)p)[0]; *b = (float)((const double *)p)[1]; break;
    case GL_UNSIGNED_SHORT: *a = ((const unsigned short *)p)[0]; *b = ((const unsigned short *)p)[1]; break;
    case GL_BYTE: *a = ((const signed char *)p)[0]; *b = ((const signed char *)p)[1]; break;
    case GL_UNSIGNED_BYTE: *a = ((const unsigned char *)p)[0]; *b = ((const unsigned char *)p)[1]; break;
    default: *a = *b = 0; break;
    }
}

/* ---- pixels ----------------------------------------------------------- */

static inline u32 mix(u32 d, u32 s, unsigned a)
{
    /* d + (s - d) * a / 255, per channel, two channels at a time */
    u32 na = 255 - a;
    u32 rb = (s & 0x00ff00ffu) * a + (d & 0x00ff00ffu) * na + 0x00800080u;
    u32 g = (s & 0x0000ff00u) * a + (d & 0x0000ff00u) * na + 0x00008000u;
    rb = ((rb + ((rb >> 8) & 0x00ff00ffu)) >> 8) & 0x00ff00ffu;
    g = ((g + ((g >> 8) & 0x0000ff00u)) >> 8) & 0x0000ff00u;
    return 0xff000000u | rb | g;
}

static inline u32 modulate(u32 t, unsigned cr, unsigned cg, unsigned cb, unsigned ca)
{
    unsigned a = ((t >> 24) * ca + 127) / 255;
    unsigned r = (((t >> 16) & 255) * cr + 127) / 255;
    unsigned g = (((t >> 8) & 255) * cg + 127) / 255;
    unsigned b = ((t & 255) * cb + 127) / 255;
    return ((u32)a << 24) | (r << 16) | (g << 8) | b;
}

static inline void put(u32 *d, u32 s, int blend)
{
    unsigned a = s >> 24;
    if (!blend || a == 255) *d = s | 0xff000000u;
    else if (a) *d = mix(*d, s, a);
}

/* a box of picture pixels [x0, x1) x [y0, y1), already clipped */
static void fill_box(int x0, int y0, int x1, int y1, u32 c, int blend)
{
    unsigned a = c >> 24;
    if (blend && a == 0) return;
    for (int y = y0; y < y1; y++) {
        u32 *p = fb + (long)y * fbpitch;
        if (!blend || a == 255) { u32 o = c | 0xff000000u; for (int x = x0; x < x1; x++) p[x] = o; }
        else for (int x = x0; x < x1; x++) p[x] = mix(p[x], c, a);
    }
}

static int pix_lo(float f) { return (int)floorf(f + 0.5f); }  /* first pixel whose centre is at or past f */

/* a rectangle: picture box fx0..fx1, fy0..fy1 and texture box u0..u1,
   v0..v1 (u0 at fx0, v0 at fy0) */
static void rect(float fx0, float fy0, float fx1, float fy1, float u0, float v0, float u1, float v1, int textured)
{
    int x0 = pix_lo(fx0), x1 = pix_lo(fx1), y0 = pix_lo(fy0), y1 = pix_lo(fy1);
    int cx0 = x0 < 0 ? 0 : x0, cy0 = y0 < 0 ? 0 : y0;
    int cx1 = x1 > fbw ? fbw : x1, cy1 = y1 > fbh ? fbh : y1;
    if (cx0 >= cx1 || cy0 >= cy1) return;
    u32 col = ((u32)C->ca << 24) | ((u32)C->cr << 16) | ((u32)C->cg << 8) | C->cb;
    const Tex *t = textured && C->bound > 0 && (int)C->bound <= ntexs ? &texs[C->bound - 1] : NULL;
    if (textured && (!t || !t->px)) return;   /* no texture: GL draws nothing useful either */
    if (!t) { fill_box(cx0, cy0, cx1, cy1, col, C->blend); return; }

    /* texel steps, 16.16, from the texture box across the picture box */
    float tw = (float)t->w, th = (float)t->h;
    float dw = fx1 - fx0, dh = fy1 - fy0;
    if (dw == 0 || dh == 0) return;
    float du = (u1 - u0) * tw / dw, dv = (v1 - v0) * th / dh;
    long fdu = (long)(du * 65536.0f), fdv = (long)(dv * 65536.0f);
    /* the texel at the first pixel's centre */
    long fu = (long)((u0 * tw + (cx0 + 0.5f - fx0) * du) * 65536.0f);
    long fv = (long)((v0 * th + (cy0 + 0.5f - fy0) * dv) * 65536.0f);
    int white = col == 0xffffffffu;
    int blend = C->blend;
    long maxu = ((long)t->w << 16) - 1, maxv = ((long)t->h << 16) - 1;

    for (int y = cy0; y < cy1; y++, fv += fdv) {
        long vv = fv < 0 ? 0 : fv > maxv ? maxv : fv;
        const u32 *src = t->px + (long)(vv >> 16) * t->w;
        u32 *d = fb + (long)y * fbpitch;
        long uu = fu;
        if (fdu == 65536 && uu >= 0 && uu + ((long)(cx1 - cx0 - 1) << 16) <= maxu) {
            /* one texel a pixel */
            const u32 *s = src + (uu >> 16);
            if (white) {
                if (!blend) { for (int x = cx0; x < cx1; x++) d[x] = *s++ | 0xff000000u; }
                else for (int x = cx0; x < cx1; x++) put(&d[x], *s++, 1);
            } else
                for (int x = cx0; x < cx1; x++) put(&d[x], modulate(*s++, C->cr, C->cg, C->cb, C->ca), blend);
            continue;
        }
        for (int x = cx0; x < cx1; x++, uu += fdu) {
            long u = uu < 0 ? 0 : uu > maxu ? maxu : uu;
            u32 s = src[u >> 16];
            if (!white) s = modulate(s, C->cr, C->cg, C->cb, C->ca);
            put(&d[x], s, blend);
        }
    }
}

/* the box round n vertices from first, and its texture box */
static void quad(int first, int n, float ax, float bx, float ay, float by, int textured)
{
    float minx = 0, maxx = 0, miny = 0, maxy = 0;
    float uL = 0, uR = 0, vT = 0, vB = 0;
    float xs[6], ys[6], us[6], vs[6];
    for (int i = 0; i < n; i++) {
        float x, y;
        fetch2(C->vp, C->vtype, C->vsize, C->vstride, first + i, &x, &y);
        xs[i] = ax * x + bx; ys[i] = ay * y + by;
        if (textured) fetch2(C->tp, C->ttype, C->tsize, C->tstride, first + i, &us[i], &vs[i]);
        if (i == 0 || xs[i] < minx) minx = xs[i];
        if (i == 0 || xs[i] > maxx) maxx = xs[i];
        if (i == 0 || ys[i] < miny) miny = ys[i];
        if (i == 0 || ys[i] > maxy) maxy = ys[i];
    }
    if (textured)
        for (int i = 0; i < n; i++) {
            if (xs[i] == minx) uL = us[i]; else uR = us[i];
            if (ys[i] == miny) vT = vs[i]; else vB = vs[i];
        }
    rect(minx, miny, maxx, maxy, uL, vT, uR, vB, textured);
}

static void line(float x0, float y0, float x1, float y1)
{
    u32 col = ((u32)C->ca << 24) | ((u32)C->cr << 16) | ((u32)C->cg << 8) | C->cb;
    if (y0 == y1 || x0 == x1) {
        /* axis aligned: the pixels whose centres the line runs through */
        int a, b, c;
        if (y0 == y1) {
            c = (int)floorf(y0); if (c < 0 || c >= fbh) return;
            if (x0 > x1) { float t = x0; x0 = x1; x1 = t; }
            a = (int)floorf(x0); b = (int)floorf(x1) + 1;
            if (a < 0) a = 0;
            if (b > fbw) b = fbw;
            if (a < b) fill_box(a, c, b, c + 1, col, C->blend);
        } else {
            c = (int)floorf(x0); if (c < 0 || c >= fbw) return;
            if (y0 > y1) { float t = y0; y0 = y1; y1 = t; }
            a = (int)floorf(y0); b = (int)floorf(y1) + 1;
            if (a < 0) a = 0;
            if (b > fbh) b = fbh;
            if (a < b) fill_box(c, a, c + 1, b, col, C->blend);
        }
        return;
    }
    float dx = x1 - x0, dy = y1 - y0;
    float n = fabsf(dx) > fabsf(dy) ? fabsf(dx) : fabsf(dy);
    int steps = (int)n;
    float sx = dx / n, sy = dy / n, x = x0, y = y0;
    for (int i = 0; i <= steps; i++, x += sx, y += sy) {
        int px = (int)floorf(x), py = (int)floorf(y);
        if (px >= 0 && px < fbw && py >= 0 && py < fbh) put(&fb[(long)py * fbpitch + px], col, C->blend);
    }
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    ensure_def();
    if (!fb || !C->vp || count <= 0) return;
    float ax, bx, ay, by;
    picture_map(&ax, &bx, &ay, &by);
    int textured = C->tex2d && C->tp;
    switch (mode) {
    case GL_TRIANGLES:
        /* two triangles a rectangle */
        for (int i = 0; i + 6 <= count; i += 6) quad(first + i, 6, ax, bx, ay, by, textured);
        break;
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLE_FAN:
    case GL_QUADS:
        for (int i = 0; i + 4 <= count; i += 4) quad(first + i, 4, ax, bx, ay, by, textured);
        break;
    case GL_LINES:
        for (int i = 0; i + 2 <= count; i += 2) {
            float x0, y0, x1, y1;
            fetch2(C->vp, C->vtype, C->vsize, C->vstride, first + i, &x0, &y0);
            fetch2(C->vp, C->vtype, C->vsize, C->vstride, first + i + 1, &x1, &y1);
            line(ax * x0 + bx, ay * y0 + by, ax * x1 + bx, ay * y1 + by);
        }
        break;
    case GL_LINE_STRIP:
    case GL_LINE_LOOP:
        for (int i = 0; i + 1 < count; i++) {
            float x0, y0, x1, y1;
            fetch2(C->vp, C->vtype, C->vsize, C->vstride, first + i, &x0, &y0);
            fetch2(C->vp, C->vtype, C->vsize, C->vstride, first + i + 1, &x1, &y1);
            line(ax * x0 + bx, ay * y0 + by, ax * x1 + bx, ay * y1 + by);
        }
        if (mode == GL_LINE_LOOP && count > 2) {
            float x0, y0, x1, y1;
            fetch2(C->vp, C->vtype, C->vsize, C->vstride, first + count - 1, &x0, &y0);
            fetch2(C->vp, C->vtype, C->vsize, C->vstride, first, &x1, &y1);
            line(ax * x0 + bx, ay * y0 + by, ax * x1 + bx, ay * y1 + by);
        }
        break;
    case GL_POINTS:
        for (int i = 0; i < count; i++) {
            float x, y;
            fetch2(C->vp, C->vtype, C->vsize, C->vstride, first + i, &x, &y);
            int px = (int)floorf(ax * x + bx), py = (int)floorf(ay * y + by);
            if (px >= 0 && px < fbw && py >= 0 && py < fbh)
                put(&fb[(long)py * fbpitch + px],
                    ((u32)C->ca << 24) | ((u32)C->cr << 16) | ((u32)C->cg << 8) | C->cb, C->blend);
        }
        break;
    default:
        C->err = GL_INVALID_ENUM;
        break;
    }
}

/* ---- textures --------------------------------------------------------- */

void glGenTextures(GLsizei n, GLuint *names)
{
    for (int i = 0; i < n; i++) {
        int k;
        for (k = 0; k < ntexs && texs[k].used; k++) {}
        if (k == ntexs) {
            Tex *nt = (Tex *)realloc(texs, (size_t)(ntexs + 8) * sizeof(Tex));
            if (!nt) { names[i] = 0; continue; }
            memset(nt + ntexs, 0, 8 * sizeof(Tex));
            texs = nt; ntexs += 8;
        }
        texs[k].used = 1;
        names[i] = (GLuint)(k + 1);
    }
}

void glDeleteTextures(GLsizei n, const GLuint *names)
{
    ensure_def();
    for (int i = 0; i < n; i++) {
        GLuint k = names[i];
        if (k == 0 || (int)k > ntexs) continue;
        free(texs[k - 1].px);
        memset(&texs[k - 1], 0, sizeof(Tex));
        if (C->bound == k) C->bound = 0;
    }
}

void glBindTexture(GLenum target, GLuint name)
{
    (void)target;
    ensure_def();
    C->bound = name;
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) { (void)target; (void)pname; (void)param; }

/* rows of RGBA (or RGB) bytes into 0xAARRGGBB */
static void convert(u32 *dst, int dpitch, const unsigned char *src, int w, int h, GLenum format)
{
    int bpp = format == GL_RGB ? 3 : 4;
    for (int y = 0; y < h; y++) {
        const unsigned char *s = src + (long)y * w * bpp;
        u32 *d = dst + (long)y * dpitch;
        if (bpp == 4)
            for (int x = 0; x < w; x++, s += 4)
                d[x] = ((u32)s[3] << 24) | ((u32)s[0] << 16) | ((u32)s[1] << 8) | s[2];
        else
            for (int x = 0; x < w; x++, s += 3)
                d[x] = 0xff000000u | ((u32)s[0] << 16) | ((u32)s[1] << 8) | s[2];
    }
}

static Tex *bound(void)
{
    if (C->bound == 0 || (int)C->bound > ntexs) return NULL;
    return &texs[C->bound - 1];
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei w, GLsizei h,
                  GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    (void)target; (void)internalformat; (void)border;
    ensure_def();
    Tex *t = bound();
    if (!t || level != 0) return;
    if (w <= 0 || h <= 0 || w > MAX_TEX || h > MAX_TEX || type != GL_UNSIGNED_BYTE) { C->err = GL_INVALID_VALUE; return; }
    if (!t->px || t->w != w || t->h != h) {
        free(t->px);
        t->px = (u32 *)calloc((size_t)w * h, 4);
        if (!t->px) { t->w = t->h = 0; C->err = GL_OUT_OF_MEMORY; return; }
        t->w = w; t->h = h;
    }
    if (pixels) convert(t->px, w, (const unsigned char *)pixels, w, h, format);
    else memset(t->px, 0, (size_t)w * h * 4);
}

void glTexSubImage2D(GLenum target, GLint level, GLint xo, GLint yo, GLsizei w, GLsizei h,
                     GLenum format, GLenum type, const GLvoid *pixels)
{
    (void)target;
    ensure_def();
    Tex *t = bound();
    if (!t || !t->px || level != 0 || !pixels || type != GL_UNSIGNED_BYTE) return;
    if (xo < 0 || yo < 0 || xo + w > t->w || yo + h > t->h) { C->err = GL_INVALID_VALUE; return; }
    convert(t->px + (long)yo * t->w + xo, t->w, (const unsigned char *)pixels, w, h, format);
}
