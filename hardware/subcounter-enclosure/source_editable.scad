// =================================================================
//  YouTube Sub-Counter Enclosure  -  64x8 / WALL-MOUNT edition
//  Fits TWO FC-16 (MAX7219) 4-in-1 LED matrix modules  ->  64x8
//  (~10 in / 256 mm of LEDs).  ESP32 lives inside; the box hangs
//  on two wall screws via integrated KEYHOLE slots.
//  Units: millimetres
// =================================================================
//  PARTS (set `part`), each -> its own STL:
//    body           one-piece shell (print DIAGONALLY on a 256 bed)
//    body_left / body_right   split halves for small beds (glue seam)
//    back_cover     rear lid: clamps boards + ESP32 mount + KEYHOLES
//    grille_test    small tile to check pixel alignment first
//    wall_template  thin drill guide - tape to wall, mark 2 holes
//    preview_assembled / preview_exploded / preview_back  (visual only)
//
//  ASSEMBLY
//    1. Drop the two 4-in-1 modules face-down into the grille pocket.
//       Wire them; leads exit the bottom slot and/or the end slots
//       (the outer pin headers tuck into the end slots).
//    2. Bolt the ESP32 to the standoffs on the back cover.
//    3. Screw the cover on - its posts clamp the boards to the grille.
//    4. Drive two screws into the wall (use wall_template to mark
//       them). Leave each head standing ~4 mm proud of the wall.
//    5. Hang: lift the box so the big holes drop over the screw
//       heads, then let it settle DOWN - the shanks ride up into the
//       narrow slots and the heads are trapped behind the panel.
// =================================================================

/* [ RENDER THIS PART ] */
part = "preview_assembled"; // [body, body_left, body_right, back_cover, grille_test, wall_template, preview_assembled, preview_exploded, preview_back]

/* [ Board / display  -  TWO FC-16 4-in-1 modules = 64x8 ] */
board_len    = 256;   // 2x 4-in-1 modules, 8x8 matrices @ 32 mm (~10 in)
board_h      = 32;    // matrix height (8 rows @ 4 mm)
board_thick  = 13;    // matrix + PCB + rear chips stack
led_area_len = 256;   // lit area length  (64 cols)
led_area_h   = 32;    // lit area height  (8 rows)

/* [ Pixel grille face ] */
grille_t   = 2.0;
cell_pitch = 4.0;   // LED spacing (256/64 = 4.0)
hole_size  = 2.9;   // window per LED. smaller = chunkier grid
hole_round = false; // true = round dots
grid_shift_x = 0;   // nudge the whole grid if it drifts from the LEDs
grid_shift_y = 0;

/* [ Body / depth ] */
clearance      = 0.4;
wall           = 2.0;
end_frame      = 8.0;   // solid frame at each END (anchors cover screws)
controller_gap = 26;    // depth behind the boards for the ESP32
                        // (board_thick+controller_gap = interior clear depth;
                        //  13+26 = 39 mm >= 1.5 in behind the grille)
back_t         = 3.0;
corner_r       = 3.0;

/* [ Cover screws ] */
screw_d      = 2.7;   // M3 self-tap (4.2 for heat-set inserts)
screw_clear  = 3.4;
screw_head_d = 6.2;
boss_d       = 7.0;

/* [ ESP32 mount (on cover) ] */
esp_mount   = true;
esp_hole_dx = 22;   // spacing across height
esp_hole_dy = 48;   // spacing along length
esp_standoff= 5;
esp_screw_d = 2.5;

/* [ Wire cutouts ] */
bottom_slot = true;   // slot in the bottom edge for the USB/data lead
cable_w     = 10;
cable_h     = 6;
end_slots   = true;   // pockets in BOTH end walls (module pin headers/wires)
end_slot_h  = 10;     // end cutout height
end_slot_d  = 9;      // end cutout depth (front-back)

/* [ WALL MOUNT  -  keyhole slots ] */
wall_mount      = true;
wall_head_d     = 8.5;  // clearance for the screw HEAD (big hole)
wall_shank_w    = 4.2;  // slot width = screw SHANK clearance
wall_slot_len   = 7;    // vertical travel of the slot
wall_hole_span  = 180;  // horizontal distance between the two keyholes
wall_hole_y     = 0;    // vertical offset of the keyholes (0 = centred)

/* [ Split fallback (small beds) ] */
split_gap = 0.15;

/* [ Quality ] */
$fn = 48;

// ------- derived -------
PL=board_len+2*clearance;  PH=board_h+2*clearance;
INNER=board_thick+controller_gap;
OUT_L=PL+2*wall+2*end_frame;  OUT_H=PH+2*wall;  OUT_D=grille_t+INNER+back_t;
BIG=OUT_L+OUT_H+OUT_D+60;  eps=0.05;
bx=OUT_L/2-end_frame/2;  by=OUT_H/2-wall-2.6;
boss_xy=[[bx,by],[bx,-by],[-bx,by],[-bx,-by]];
post_y=PH/2-1.4;  post_x=[-PL/2+8,-PL/6,PL/6,PL/2-8];

module rrect(l,h,r){ minkowski(){ square([max(.01,l-2*r),max(.01,h-2*r)],center=true); circle(r);} }

module grille_holes(){
  cols=round(led_area_len/cell_pitch); rows=round(led_area_h/cell_pitch);
  for(cx=[0:cols-1]) for(ry=[0:rows-1]){
    x=(cx+0.5)*cell_pitch-led_area_len/2+grid_shift_x;
    y=(ry+0.5)*cell_pitch-led_area_h/2+grid_shift_y;
    translate([x,y,-eps])
      if(hole_round) cylinder(h=grille_t+2*eps,d=hole_size);
      else translate([-hole_size/2,-hole_size/2,0]) cube([hole_size,hole_size,grille_t+2*eps]);
  }
}

module body(){
  difference(){
    union(){
      difference(){
        linear_extrude(OUT_D) rrect(OUT_L,OUT_H,corner_r);
        translate([0,0,grille_t]) linear_extrude(OUT_D) rrect(PL,PH,max(0.5,corner_r-wall));
      }
      for(p=boss_xy) translate([p[0],p[1],grille_t]) cylinder(h=INNER,d=boss_d);
    }
    grille_holes();
    for(p=boss_xy) translate([p[0],p[1],grille_t-eps]) cylinder(h=OUT_D,d=screw_d);
    if(bottom_slot)
      translate([-cable_w/2,-OUT_H/2-eps,grille_t+board_thick+controller_gap/2-cable_h/2])
        cube([cable_w,wall+2*eps,cable_h]);
    if(end_slots) for(sx=[-1,1])
      translate([sx*(OUT_L/2)-((sx<0)?eps:wall+end_frame+1-eps),-end_slot_h/2,
                 grille_t+board_thick+controller_gap/2-end_slot_d/2])
        cube([wall+end_frame+1,end_slot_h,end_slot_d]);
  }
}

// keyhole cut: big hole at bottom (head passes), narrow slot up (shank rides,
// head trapped). Cut all the way through the back panel.
module keyhole_cut(depth){
  s=wall_slot_len/2;
  union(){
    translate([0,-s,0]) cylinder(h=depth,d=wall_head_d);           // head entry
    hull(){                                                        // shank slot
      translate([0,-s,0]) cylinder(h=depth,d=wall_shank_w);
      translate([0, s,0]) cylinder(h=depth,d=wall_shank_w);
    }
  }
}

module back_cover(){
  z0=OUT_D-back_t;
  difference(){
    union(){
      translate([0,0,z0]) linear_extrude(back_t) rrect(OUT_L,OUT_H,corner_r);
      for(sy=[-1,1]) for(x=post_x)
        translate([x,sy*post_y,grille_t+board_thick]) cylinder(h=z0-(grille_t+board_thick),d=4);
      if(esp_mount) for(sx=[-1,1]) for(sy=[-1,1])
        translate([sy*esp_hole_dy/2,sx*esp_hole_dx/2,z0-esp_standoff])
          difference(){ cylinder(h=esp_standoff,d=6); translate([0,0,-eps]) cylinder(h=esp_standoff+2*eps,d=esp_screw_d);}
    }
    for(p=boss_xy){
      translate([p[0],p[1],z0-eps]) cylinder(h=back_t+2*eps,d=screw_clear);
      translate([p[0],p[1],OUT_D-1.6]) cylinder(h=2,d=screw_head_d);
    }
    if(wall_mount) for(sx=[-1,1])
      translate([sx*wall_hole_span/2,wall_hole_y,z0-eps]) keyhole_cut(back_t+2*eps);
    if(bottom_slot)
      translate([-cable_w/2,-OUT_H/2-eps,OUT_D-cable_h]) cube([cable_w,wall+2*eps,cable_h]);
  }
}

module grille_test(){          // leftmost 8 columns x 8 rows, lay on one lit matrix
  w=cell_pitch*8;
  difference(){
    translate([-led_area_len/2,-OUT_H/2,0]) cube([w,OUT_H,grille_t]);
    grille_holes();
  }
}

// flat drill guide: tape to wall, mark the two SEATED screw positions
module wall_template(){
  t=2; s=wall_slot_len/2;
  difference(){
    linear_extrude(t) rrect(wall_hole_span+24,OUT_H,corner_r);
    for(sx=[-1,1]) translate([sx*wall_hole_span/2,wall_hole_y+s,-eps]) cylinder(h=t+2*eps,d=3.2);
    for(sx=[-1,1]) translate([sx*(wall_hole_span/2+9),wall_hole_y+s,-eps]) cylinder(h=t+2*eps,d=1.2);
  }
}

module split_keep(sd){ intersection(){ children(); translate([sd*(BIG/2+split_gap/2),0,0]) cube(BIG,center=true);} }

if(part=="body") body();
else if(part=="body_left")  split_keep(-1) body();
else if(part=="body_right") split_keep( 1) body();
else if(part=="back_cover") back_cover();
else if(part=="grille_test") grille_test();
else if(part=="wall_template") wall_template();
else if(part=="preview_assembled"){
  color("SteelBlue") body();
  color("DimGray",0.55) back_cover();
  color("red",0.5) translate([0,0,grille_t+0.2]) linear_extrude(0.6) square([led_area_len,led_area_h],center=true);
}
else if(part=="preview_exploded"){
  color("SteelBlue") body();
  color("DimGray") translate([0,0,50]) back_cover();
}
else if(part=="preview_back"){
  color("DimGray") back_cover();
}
