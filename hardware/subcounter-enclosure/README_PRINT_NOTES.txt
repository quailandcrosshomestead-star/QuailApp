YOUTUBE SUB-COUNTER ENCLOSURE  -  32x8 / WALL-MOUNT edition
===========================================================
Fits ONE FC-16 (MAX7219) 4-in-1 LED matrix module  (32x8 display,
~128 mm of LEDs).  ESP32 mounts inside on the rear cover.  The box
hangs on two wall screws via built-in keyhole slots.

OUTSIDE SIZE (approx):  149 x 37 x 44 mm  (L x H x depth = 1.73 in)
INTERIOR CLEAR DEPTH:   ~39 mm (1.54 in) behind the grille face

WHAT TO PRINT & HOW MANY
------------------------
  body.stl ............. x1   (deep shell with the pixel-grille face)
  back_cover.stl ....... x1   (rear lid: clamp posts + ESP32 mount + keyholes)
  wall_template.stl .... x1   (OPTIONAL - a flat drill guide for the wall)
  grille_test.stl ...... x1   (OPTIONAL, print this FIRST - see note)

PRINT SETTINGS (any FDM printer)
--------------------------------
  Material ......... PLA indoors. PETG/ASA if it lives in sun/heat.
  Layer height ..... 0.2 mm
  Nozzle ........... 0.4 mm
  Walls ............ 3        Infill ..... 15-20%
  Supports ......... NONE if oriented as below.
  Do NOT scale the parts - scaling throws off the LED pixel spacing.
  Fits any bed >=150 mm; on a 150 mm bed lay the body diagonally.

ORIENTATION
-----------
  body ......... GRILLE FACE DOWN on the plate (crisp holes, cavity up).
  back_cover ... flat, post/standoff side UP.
  wall_template. flat.

HARDWARE (buyer supplies)
-------------------------
  M3 screws:
    - 4x ~10 mm  fasten the back cover to the body (self-tap into the
      printed bosses, OR set screw_d=4.2 in the source for M3 inserts).
  Wall screws:
    - 2x #6 / M3.5-M4 wood or drywall screws (pan/round head ~7-8 mm).
      Drive them so the HEAD stands ~4 mm proud of the wall.

ASSEMBLY
--------
  1. Drop the 4-in-1 module face-down into the grille pocket.  Its pin
     headers tuck into the slots in the two end walls; the data/power
     leads exit the bottom slot (and/or an end slot).
  2. Bolt the ESP32 to the 4 standoffs on the back cover.
  3. Screw the cover on - its posts clamp the board against the grille.

WALL MOUNTING
-------------
  1. Tape wall_template to the wall, level it (the two tiny side notches
     mark the screw line), and mark/drill the 2 holes 90 mm apart.
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

Every dimension is a labelled variable in subcounter_32x8_wall.scad
(OpenSCAD).  Key ones: board_len/board_h/board_thick, cell_pitch,
hole_size, controller_gap, and the [WALL MOUNT] group.
