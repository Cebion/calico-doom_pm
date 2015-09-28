/*
  CALICO

  Renderer phase 3 - Sprite prep
*/

#include "doomdef.h"
#include "r_local.h"

//
// Project vissprite for potentially visible actor
//
static void R_PrepMobj(mobj_t *thing)
{
   fixed_t tr_x, tr_y;
   fixed_t gxt, gyt;
   fixed_t tx, tz;
   fixed_t xscale;

   spritedef_t   *sprdef;
   spriteframe_t *sprframe;

   angle_t      ang;
   unsigned int rot;
   boolean      flip;
   int          lump;
   vissprite_t *vis;

   // transform origin relative to viewpoint
   tr_x = thing->x - viewx;
   tr_y = thing->y - viewy;

   gxt =  FixedMul(tr_x, viewcos);
   gyt = -FixedMul(tr_y, viewsin);
   tz  = gxt - gyt;

   // thing is behind view plane?
   if(tz < MINZ)
      return;

   gxt = -FixedMul(tr_x, viewsin);
   gyt =  FixedMul(tr_y, viewcos);
   tx  = -(gyt + gxt);

   // too far off the side?
   if(tx > (tz << 2) || tx < -(tz<<2))
      return;

   // check sprite for validity
   if((unsigned int)thing->spawnangle >= NUMSPRITES)
      return;

   sprdef = &sprites[thing->sprite];

   // check frame for validity
   if((thing->frame & FF_FRAMEMASK) >= sprdef->numframes)
      return;

   sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

   if(sprframe->rotate)
   {
      // select proper rotation depending on player's view point
      ang  = R_PointToAngle2(viewx, viewy, thing->x, thing->y);
      rot  = (ang - thing->angle + (unsigned int)(ANG45 / 2)*9) >> 29;
      lump = sprframe->lump[rot];
      flip = (boolean)(sprframe->flip[rot]);
   }
   else
   {
      // sprite has a single view for all rotations
      lump = sprframe->lump[0];
      flip = (boolean)(sprframe->flip[0]);
   }

   // get a new vissprite
   if(vissprite_p == vissprites + MAXVISSPRITES)
      return; // too many visible sprites already
   vis = vissprite_p++;

   vis->patchnum = lump; // CALICO: store to patchnum, not patch (number vs pointer)
   vis->x1       = tx;
   vis->gx       = thing->x;
   vis->gy       = thing->y;
   vis->gz       = thing->z;
   vis->xscale   = xscale = FixedDiv(PROJECTION, tz);
   vis->yscale   = FixedMul(xscale, STRETCH);
   vis->yiscale  = FixedDiv(FRACUNIT, vis->yscale); // NB: -1 in GAS... test w/o.

   if(flip)
      vis->xiscale = -FixedDiv(FRACUNIT, xscale);
   else
      vis->xiscale = FixedDiv(FRACUNIT, xscale);

   if(thing->frame & FF_FULLBRIGHT)
      vis->colormap = 255;
   else
      vis->colormap = thing->subsector->sector->lightlevel;
}

//
// Project player weapon sprite
//
static void R_PrepPSprite(pspdef_t *psp)
{
   spritedef_t   *sprdef;
   spriteframe_t *sprframe;
   int            lump;
   vissprite_t   *vis;
/*
 subq #12,FP

 move FP,r0
 addq #4,r0 ; &sprdef                          // &sprdef    => r0
 load (FP+3),r1 ; local psp                    // psp (arg)  => r1
 load (r1),r2                                  // psp->state => r2
 load (r2),r2                                  // state->sprite => r2
 shlq #3,r2                                    // adjust to array index
 movei #_sprites,r3                            // sprites => r3
 add r3,r2                                     // &sprites[r2] => r2
 store r2,(r0)                                 // r2 => *r0 (sprdef)
 move FP,r2
 addq #8,r2 ; &sprframe                        // &sprframe => r2
 movei #44,r3                                  // sizeof(spriteframe_t) => r3
 load (r1),r1                                  // psp->state => r1
 moveq #4,r4                                   // &state_t::frame => r4
 add r4,r1                                     // r1 += r4
 load (r1),r1                                  // state->frame => r1
 movei #32767,r5                               // FF_FRAMEMASK => r5
 and r5,r1                                     // r1 &= r5
 move r3,MATH_A
 movei #L135,MATH_RTS
 movei #GPU_IMUL,scratch
 jump T,(scratch)
 move r1,MATH_B ; delay slot
L135:
 move MATH_C,r3                                // r3*r1 => r3
 load (r0),r0                                  // *r0 => r0
 add r4,r0                                     // r0 += &spritedef_t::spriteframes
 load (r0),r0                                  // *r0 => r0
 move r3,r1                                    // r3 => r1
 add r0,r1                                     // r1 += r0 (sprdef->spriteframes[r3])
 store r1,(r2)                                 // r1 => *r2 (sprframe)
 move FP,r0 ; &lump                            // &lump => r0
 load (r2),r1                                  // sprframe => r1
 add r4,r1                                     // r1 += &spriteframe_t::lump
 load (r1),r1                                  // *r1 => r1
 store r1,(r0)                                 // r1 => *r0 (lump = sprframe->lump[0])
 movei #_vissprite_p,r0                        // &vissprite_p => r0
 load (r0),r0                                  // *r0 => r0
 move r0,r15 ;(vis)                            // r0 => r15 (vis)
 move r15,r0 ;(vis)                            // r15 => r0 (vis)
 movei #_vissprites+7680,r1                    // vissprites + NUMVISSPRITES * sizeof(vissprite_t) => r1
 cmp r0,r1
 movei #L130,scratch
 jump NE,(scratch)                             // if !=, goto L130
 nop

 movei #L129,r0                                // goto L129 unconditional (out of vissprites)
 jump T,(r0)
 nop

L130:
 movei #_vissprite_p,r0                        // &vissprite_p => r0
 movei #60,r1                                  // sizeof(vissprite_t) => r1
 move r15,r2 ;(vis)                            // r15 => r2 (vis)
 add r1,r2                                     // r2 += r1
 store r2,(r0)                                 // r1 => vissprite_p
 move r15,r0 ;(vis)                            // r15 => r0 (vis)
 addq #32,r0                                   // r0 += &vissprite_t::patch
 load (FP),r1 ; local lump                     // lump => r1
 store r1,(r0)                                 // r1 => vis->patch

 load (FP+3),r0 ; local psp                    // psp => r0
 addq #8,r0                                    // r0 += &pspdef_t::sx
 load (r0),r0                                  // psp->sx => r0
 sharq #16,r0                                  // r0 >>= FRACBITS
 store r0,(r15) ;(vis)                         // r0 => vis->x1

 move r15,r0 ;(vis)                            // r15 => r0 (vis)
 addq #28,r0                                   // r0 += &vissprite_t::texturemid
 load (FP+3),r1 ; local psp                    // psp => r1
 addq #12,r1                                   // r1 += &pspdef_t::sy
 load (r1),r1                                  // psp->sy => r1
 store r1,(r0)                                 // r1 => vis->texturemid

 load (FP+3),r0 ; local psp                    // psp => r0
 load (r0),r0                                  // psp->state => r0
 addq #4,r0                                    // r0 += &state_t::frame
 load (r0),r0                                  // psp->state->frame => r0
 movei #32768,r1                               // FF_FULLBRIGHT => r1  
 and r1,r0                                     // r0 &= r1
 moveq #0,r1                                   // 0 => r1
 cmp r0,r1
 movei #L133,scratch
 jump EQ,(scratch)                             // if !(frame & FF_FULLBRIGHT), goto L133
 nop

 movei #36,r0                                  // &vissprite_t::colormap => r0
 move r15,r1 ;(vis)                            // vis => r1
 add r0,r1                                     // r1 += r0
 movei #255,r0                                 // 255 => r0
 store r0,(r1)                                 // r0 => vis->colormap

 movei #L134,r0                                // goto L134 (unconditional)
 jump T,(r0)
 nop

L133:                                        // COND: not fullbright
 movei #36,r0                                  // &vissprite_t::colormap => r0
 move r15,r1 ;(vis)                            // vis => r1
 add r0,r1                                     // r1 += r0
 movei #_viewplayer,r0                         // &viewplayer => r0
 load (r0),r0                                  // *r0 => r0
 load (r0),r0                                  // viewplayer->mo => r0
 movei #52,r2                                  // &mobj_t::subsector => r2
 add r2,r0                                     // r0 += r2
 load (r0),r0                                  // mo->subsector => r0
 load (r0),r0                                  // subsector->sector => r0
 addq #16,r0                                   // r0 += &sector_t::lightlevel
 load (r0),r0                                  // sector->lightlevel => r0
 store r0,(r1)                                 // r0 => vis->colormap

L134:
L129:
 jump T,(RETURNPOINT)
 addq #12,FP ; delay slot
*/
}

//
// Process actors in all visible subsectors
//
void R_SpritePrep(void)
{
   subsector_t **ssp = vissubsectors;
   pspdef_t     *psp;
   int i;

   while(ssp < lastvissubsector)
   {
      subsector_t *ss = *ssp;
      sector_t    *se = ss->sector;

      if(se->validcount != validcount) // not already processed?
      {
         mobj_t *thing = se->thinglist;
         se->validcount = validcount;  // mark it as processed

         while(thing) // walk sector thing list
         {
            R_PrepMobj(thing);
            thing = thing->snext;
         }
      }
      ++ssp;
   }

   // remember end of actor vissprites
   lastsprite_p = vissprite_p;

   // draw player weapon sprites
   for(i = 0, psp = viewplayer->psprites; i < NUMPSPRITES; i++, psp++)
   {
      if(psp->state)
         R_PrepPSprite(psp);
   }
}

// EOF

/*
#define	AC_ADDFLOOR      1       000:00000001
#define	AC_ADDCEILING    2       000:00000010
#define	AC_TOPTEXTURE    4       000:00000100
#define	AC_BOTTOMTEXTURE 8       000:00001000
#define	AC_NEWCEILING    16      000:00010000
#define	AC_NEWFLOOR      32      000:00100000
#define	AC_ADDSKY        64      000:01000000
#define	AC_CALCTEXTURE   128     000:10000000
#define	AC_TOPSIL        256     001:00000000
#define	AC_BOTTOMSIL     512     010:00000000
#define	AC_SOLIDSIL      1024    100:00000000

typedef struct
{
   // filled in by bsp
  0:   seg_t        *seg;
  4:   int           start;
  8:   int           stop;   // inclusive x coordinates
 12:   int           angle1; // polar angle to start

   // filled in by late prep
 16:   pixel_t      *floorpic;
 20:   pixel_t      *ceilingpic;

   // filled in by early prep
 24:   unsigned int  actionbits;
 28:   int           t_topheight;
 32:   int           t_bottomheight;
 36:   int           t_texturemid;
 40:   texture_t    *t_texture;
 44:   int           b_topheight;
 48:   int           b_bottomheight;
 52:   int           b_texturemid;
 56:   texture_t    *b_texture;
 60:   int           floorheight;
 64:   int           floornewheight;
 68:   int           ceilingheight;
 72:   int           ceilingnewheight;
 76:   byte         *topsil;
 80:   byte         *bottomsil;
 84:   unsigned int  scalefrac;
 88:   unsigned int  scale2;
 92:   int           scalestep;
 96:   unsigned int  centerangle;
100:   unsigned int  offset;
104:   unsigned int  distance;
108:   unsigned int  seglightlevel;
} viswall_t;

typedef struct seg_s
{
 0:   vertex_t *v1;
 4:   vertex_t *v2;
 8:   fixed_t   offset;
12:   angle_t   angle;       // this is not used (keep for padding)
16:   side_t   *sidedef;
20:   line_t   *linedef;
24:   sector_t *frontsector;
28:   sector_t *backsector;  // NULL for one sided lines
} seg_t;

typedef struct line_s
{
 0: vertex_t     *v1;
 4: vertex_t     *v2;
 8: fixed_t      dx;
12: fixed_t      dy;                    // v2 - v1 for side checking
16: VINT         flags;
20: VINT         special;
24: VINT         tag;
28: VINT         sidenum[2];               // sidenum[1] will be -1 if one sided
36: fixed_t      bbox[4];
52: slopetype_t  slopetype;                // to aid move clipping
56: sector_t    *frontsector;
60: sector_t    *backsector;
64: int          validcount;               // if == validcount, already checked
68: void        *specialdata;              // thinker_t for reversable actions
72: int          fineangle;                // to get sine / cosine for sliding
} line_t;

typedef struct
{
 0:  fixed_t   textureoffset; // add this to the calculated texture col
 4:  fixed_t   rowoffset;     // add this to the calculated texture top
 8:  VINT      toptexture;
12:  VINT      bottomtexture;
16:  VINT      midtexture;
20:  sector_t *sector;
} side_t;

typedef	struct
{
 0:   fixed_t floorheight;
 4:   fixed_t ceilingheight;
 8:   VINT    floorpic;
12:   VINT    ceilingpic;        // if ceilingpic == -1,draw sky
16:   VINT    lightlevel;
20:   VINT    special;
24:   VINT    tag;
28:   VINT    soundtraversed;              // 0 = untraversed, 1,2 = sndlines -1
32:   mobj_t *soundtarget;                 // thing that made a sound (or null)
36:   VINT        blockbox[4];             // mapblock bounding box for height changes
52:   degenmobj_t soundorg;                // for any sounds played by the sector
76:   int     validcount;                  // if == validcount, already checked
80:   mobj_t *thinglist;                   // list of mobjs in sector
84:   void   *specialdata;                 // thinker_t for reversable actions
88:   VINT    linecount;
92:   struct line_s **lines;               // [linecount] size
} sector_t;

typedef struct subsector_s
{
0:   sector_t *sector;
4:   VINT      numlines;
8:   VINT      firstline;
} subsector_t;

typedef struct
{
 0:  char     name[8];  // for switch changing, etc
 8:  int      width;
12:  int      height;
16:  pixel_t *data;     // cached data to draw from
20:  int      lumpnum;
24:  int      usecount; // for precaching
28:  int      pad;
} texture_t;

typedef struct mobj_s
{
  0: struct mobj_s *prev;
  4: struct mobj_s *next;
  8: latecall_t     latecall;
 12: fixed_t        x;
 16: fixed_t        y;
 20: fixed_t        z;
 24: struct mobj_s *snext;
 28: struct mobj_s *sprev;
 32: angle_t        angle;
 36: VINT           sprite;
 40: VINT           frame;
 44: struct mobj_s *bnext;
 48: struct mobj_s *bprev;
 52: struct subsector_s *subsector;
 56: fixed_t        floorz;
 60: fixed_t        ceilingz;
 64: fixed_t        radius;
 68: fixed_t        height;
 72: fixed_t        momx;
 76: fixed_t        momy;
 80: fixed_t        momz;
 84: mobjtype_t     type;
 88: mobjinfo_t    *info;
 92: VINT           tics;
 96: state_t       *state;
100: int            flags;
104: VINT           health;
108: VINT           movedir;
112: VINT           movecount;
116: struct mobj_s *target;
120: VINT           reactiontime;
124: VINT           threshold;
132: struct player_s *player;
136: struct line_s *extradata;
140: short spawnx;
142: short spawny;
144: short spawntype;
146: short spawnangle;
} mobj_t;

typedef struct vissprite_s
{
 0:  int     x1;
 4:  int     x2;
 8:  fixed_t startfrac;
12:  fixed_t xscale;
16:  fixed_t xiscale;
20:  fixed_t yscale;
24:  fixed_t yiscale;
28:  fixed_t texturemid;
32:  patch_t *patch;
36:  int     colormap;
40:  fixed_t gx;
44:  fixed_t gy;
48:  fixed_t gz;
52:  fixed_t gzt;
56:  pixel_t *pixels;
} vissprite_t;

typedef struct
{
 0:  state_t *state;
 4:  int      tics;
 8:  fixed_t  sx;
12:  fixed_t  sy;
} pspdef_t;

typedef struct
{
 0:  spritenum_t sprite;
 4:  long        frame;
 8:  long        tics;
12:  void        (*action)();
16:  statenum_t  nextstate;
20:  long        misc1;
24:  long        misc2;
} state_t;

typedef struct
{
 0:  boolean rotate;  // if false use 0 for any position
 4:  int     lump[8]; // lump to use for view angles 0-7
36:  byte    flip[8]; // flip (1 = flip) to use for view angles 0-7
} spriteframe_t;

typedef struct
{
0: int            numframes;
4: spriteframe_t *spriteframes;
} spritedef_t;
*/

