YOUTUBE SUB-COUNTER ENCLOSURE  -  64x8 / WALL-MOUNT edition
===========================================================
Fits TWO FC-16 (MAX7219) 4-in-1 LED matrix modules  (64x8 display,
~10 in / 256 mm of LEDs).  ESP32 mounts inside on the rear cover.
The box hangs on two wall screws via built-in keyhole slots.

OUTSIDE SIZE (approx):  277 x 37 x 44 mm  (L x H x depth)
                        = 10.9 x 1.45 x 1.73 in
INTERIOR CLEAR DEPTH:   ~39 mm (1.54 in) behind the grille face

WHAT TO PRINT & HOW MANY
------------------------
  body.stl ............. x1   (deep shell with the pixel-grille face)
  back_cover.stl ....... x1   (rear lid: clamp posts + ESP32 mount + keyholes)
  wall_template.stl .... x1   (OPTIONAL - a flat drill guide for the wall)
  grille_test.stl ...... x1   (OPTIONAL, print this FIRST - see note)
  --- only if your bed is smaller than ~260 mm: ---
  body_left.stl + body_right.stl .. x1 each  (halves; glue the seam)

PRINT SETTINGS (any FDM printer)
--------------------------------
  Material ......... PLA indoors. PETG/ASA if it lives in sun/heat.
  Layer height ..... 0.2 mm
  Nozzle ........... 0.4 mm
  Walls ............ 3        Infill ..... 15-20%
  Supports ......... NONE if oriented as below.
  Do NOT scale the parts - scaling throws off the LED pixel spacing.

BED SIZE  (the body is ~277 mm long)
------------------------------------
  * 256 mm bed (Bambu A1/P1/X1, Ender, etc.): place the body
    DIAGONALLY - it fits.
  * Bed smaller than ~260 mm and can't go diagonal: print
    body_left.stl + body_right.stl instead and glue the seam
    (a faint grille seam will show at the join).

ORIENTATION
-----------
  body ......... GRILLE FACE DOWN on the plate (crisp holes, cavity up).
  back_cover ... flat, post/standoff side UP.
  halves ....... grille face down, flat cut edge toward the plate centre.
  wall_template. flat.

HARDWARE (buyer supplies)
-------------------------
  M3 screws:
    - 4x ~10 mm  fasten the back cover to the body (self-tap into the
      printed bosses, OR set screw_d=4.2 in the source for M3 inserts).
  Wall screws:
    - 2x #6 / M3.5-M4 wood or drywall screws (pan/round head ~7-8 mm),
      180 mm apart.  Drive them so each HEAD stands ~4 mm off the wall.

ASSEMBLY
--------
  1. Drop the two 4-in-1 modules face-down into the grille pocket.  The
     outer pin headers tuck into the slots in the two end walls; the
     data/power leads exit the bottom slot (and/or an end slot).
  2. Bolt the ESP32 to the 4 standoffs on the back cover.
  3. Screw the cover on - its posts clamp the boards against the grille.

WALL MOUNTING
-------------
  1. Tape wall_template to the wall, level it (the two tiny side notches
     mark the screw line), and mark/drill the 2 holes 180 mm apart.
  2. Drive the 2 wall screws; leave each head ~4 mm off the wall.
  3. Lift the box so the big round part of each keyhole drops over a
     screw head, then let it settle DOWN - the shanks slide up into the
     narrow slots and the heads are trapped behind the panel.
  Prefer a different screw spacing? Change wall_hole_span in the source.

TIPS
----
  * A BLACK body/front makes the grid between pixels vanish, so it reads
    like a real LED sign.
  * Print grille_test.stl first and lay it over ONE lit 8x8 matrix to
    confirm the 4.0 mm pixel pitch lines up before printing the big body.

Every dimension is a labelled variable in subcounter_64x8_wall.scad
(OpenSCAD).  Key ones: board_len/board_h/board_thick, cell_pitch,
hole_size, controller_gap, and the [WALL MOUNT] group.
