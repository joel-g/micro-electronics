# Dual-gauge enclosure — revision 8 split front and floor

A compact dashboard enclosure for two 1.28-inch round TFT displays, an ESP32,
CAN adapter, compass/magnetometer and gyro. The current design is **revision 8**:
a separate front and floor joined with three M2 bolts and captive nuts, plus a
removable cover. It was developed for a Honda CR-V mounting location; check the
board dimensions below against your own hardware.
Print the front with its visible face on the bed, and the floor bottom-down with
its electronics mounts upward. No full L-piece needs to be printed on an edge.

Use the [revision 8 STL list](#current-revision-8-files) for the downloadable
parts. The same enclosure fits the two-display assembly running either
[Dual Gauge with Buttons](../../dual-gauge-with-buttons/README.md) or
[Dual Gauge CAN](../../dual-gauge-can/README.md). Use the corresponding
project guide for electronics and wiring.

## Why revision 8 uses separate pieces

The earlier one-piece L-shaped chassis did not print well on the prototype's
**Creality Ender 3**. Printing floor-down left an unsatisfactory visible front
finish; printing face-down improved that surface but still did not give a good
overall print. Revision 7 corrected the small floor edge that prevented the face
from lying flat, but changing orientation alone did not resolve the print-quality
problem.

**This is why revision 8 separates the front and floor.** Each can now be printed
in its preferred orientation: the visible front face against the bed, and the
floor bottom-down with electronics mounts growing upward. They are then bolted
together. This records the prototype's Ender 3 experience, not a claim that the
older L-pieces cannot work on other printers. Revision 8's physical print results
have not yet been confirmed.

## Start here: what to print

For a new enclosure, print **one front, one floor and one cover** from this list.
Do not print every STL in this directory; many are older designs or duplicates.


- **`front_v8_print.stl`** — complete display/button face, screen mounts and three
  rear joining tabs. Already face-down at Z=0; keep its supplied orientation.
- **`floor_v8_print.stl`** — complete separate floor, electronics mounts, three
  joining pads and cover-fastening holes. Already floor-down at Z=0.
- **`cover_v8_print.stl`** — top, sides and rear shell. Already oriented with its
  outside top on the bed and its open bottom upward. Skip this print if you
  already have a revision 5, 6 or 7 cover; those covers fit revision 8.

Print at 100% scale in millimeters. Check the slicer preview for local bridges
and overhangs, particularly the small transverse holes and nut pockets in the
front tabs. The broad front face rests directly on the bed. These exports have
passed geometry checks; revision 8 has not yet been sliced or physically fitted.

**Optional quick joint trial:** print one `joint_front_v8_test.stl` and one
`joint_floor_v8_test.stl`. These are cropped copies of the central joint, with
exactly the same pockets, clearances and print orientations as the full parts.
They are test coupons, not extra pieces needed in the final enclosure.

`front_v8_assembly.stl` and `floor_v8_assembly.stl` use assembled coordinates for
CAD inspection. They duplicate the main parts and are not additional prints.
All earlier `chassis_v*.stl` files and generic `chassis.stl` are historical
one-piece chassis designs; use the two named revision 8 print files above.

## STL filename guide

- **`_print`**: exported in a deliberate print orientation. Preserve that
  orientation when importing into a slicer.
- **`_assembly`**: the same part in its installed position and shared CAD
  coordinates. It is **not an additional part or a complete assembly**. Import
  these into CAD without individually centering them to inspect how they mate.
- **`_test` / `_fit_check`**: optional small test pieces, also called coupons,
  used to check clearances before committing to a full print.
- **`chassis`**: the older, combined L-shaped front and floor.
- **`_m2`**: screen fasteners use M2 through-bolts and nuts with recessed heads.
- **`_pilot`**: an older alternative with small blind holes intended for screws
  biting into plastic. These are not the M2 through-bolt version.
- **`vN`**: design revision, not the number of copies to print.

STL files do not encode units: import these as **millimeters at 100% scale**.

### Current revision 8 files

| STL | Contents | Use / quantity |
| --- | --- | --- |
| [front_v8_print.stl](front_v8_print.stl) | Full front, screen mounts and rear joining tabs. | **Print 1**, visible face down. |
| [floor_v8_print.stl](floor_v8_print.stl) | Full floor, electronics mounts and joint/cover fastening provisions. | **Print 1**, bottom down, mounts up. |
| [cover_v8_print.stl](cover_v8_print.stl) | Removable top, two sides and back, with USB and rear cable openings. | **Print 1**, outer top down; reuse a v5–v7 cover if available. |
| [front_v8_assembly.stl](front_v8_assembly.stl) | Same front as `front_v8_print.stl`, in its installed orientation. | CAD reference only; **no extra print**. |
| [floor_v8_assembly.stl](floor_v8_assembly.stl) | Same floor as `floor_v8_print.stl`, in shared assembly coordinates. | CAD reference only; **no extra print**. |
| [joint_front_v8_test.stl](joint_front_v8_test.stl) | Small crop of the central front tab, nut pocket and face. | Optional: print 1 with the floor coupon to check the new joint. |
| [joint_floor_v8_test.stl](joint_floor_v8_test.stl) | Matching small crop of the floor pad, locating edges and recessed bolt hole. | Optional: print 1 with the front coupon. |

The two joint coupons are already oriented for printing. They do not attach to
or replace any full-size part. If testing first, assemble them using one M2×6
bolt, one nut and an optional washer, then print the full front and floor once
satisfied with the fit.

To inspect the full enclosure in CAD, import the two v8 `_assembly` files and
`cover_v7.stl` together while preserving their coordinates. There is no separate
v8 cover assembly export because its geometry matches the v7 cover.

### Historical one-piece chassis files

These are retained for reproducibility. They are **alternatives to**, not
components of, the current split design. Their front and floor are one print;
a v8 front or floor cannot be bolted onto them as a replacement half.

| STL | Revision / difference | Supplied orientation |
| --- | --- | --- |
| [chassis_v3_m2.stl](chassis_v3_m2.stl) | Early L-piece with M2 screen recesses; 18.50 mm screen screw spacing and 0.50 mm downward correction. | Floor down. |
| [chassis_v3_pilot.stl](chassis_v3_pilot.stl) | Same revision, with blind screen pilot holes instead of M2 through holes. | Floor down. |
| [chassis_v4_m2.stl](chassis_v4_m2.stl) | Screen spacing increased to 18.75 mm; cumulative downward correction 0.75 mm. | Floor down. |
| [chassis_v4_pilot.stl](chassis_v4_pilot.stl) | Revision 4 with blind screen pilot holes. | Floor down. |
| [chassis_v5_m2.stl](chassis_v5_m2.stl) | Added electronics mounts and cable restraints; final 18.85 mm screen spacing. Raised CAN and sensor seats. | Floor down. |
| [chassis_v6_m2.stl](chassis_v6_m2.stl) | Lowered CAN, compass and gyro to 1.5 mm underside clearance; CAN moved inward. | Floor down. |
| [chassis_v7_m2.stl](chassis_v7_m2.stl) | Revision 6 layout with projecting floor edge trimmed flush to the tilted face. | Floor down / assembly coordinates. |
| [chassis_v7_m2_face_down.stl](chassis_v7_m2_face_down.stl) | **Same shape** as `chassis_v7_m2.stl`, rotated for printing on the front. | Visible face down. |

Revisions 3–4 predate the electronics layout and later cover fastening pattern.
Revisions 5–7 use the same cover geometry as revision 8. The revision 8 CAN
cradle returns to the raised v5 arrangement; compass and gyro remain low.

### Historical cover files

Print **one** appropriate cover, not both its coordinate variants. A cover
without `_print` is in assembly coordinates, with its open bottom downward.

| STL | Purpose / compatibility |
| --- | --- |
| [cover_v4.stl](cover_v4.stl) | Pre-interior-layout cover in assembly coordinates. Does **not** fit the v5–v8 fastening pattern. |
| [cover_v4_print.stl](cover_v4_print.stl) | Same v4 cover oriented outer-top-down for printing. |
| [cover_v5.stl](cover_v5.stl) | Current-compatible cover shape in assembly coordinates; added rear cable notch and relocated fasteners. |
| [cover_v5_print.stl](cover_v5_print.stl) | Same v5 cover oriented for printing. |
| [cover_v6.stl](cover_v6.stl) | Identical to `cover_v5.stl`. |
| [cover_v6_print.stl](cover_v6_print.stl) | Identical to `cover_v5_print.stl`. |
| [cover_v7.stl](cover_v7.stl) | Identical to `cover_v5.stl`; use for CAD alignment with the v8 assembly files. |
| [cover_v7_print.stl](cover_v7_print.stl) | Identical to `cover_v5_print.stl`. |

`cover_v8_print.stl` has the same cover geometry, with different X/Y placement
on the print bed. It is compatible with the revision 5–8 chassis designs.

### Historical fit-test pieces

These were intermediate checks for the earlier L-piece. They **do not contain
the revision 8 front-to-floor joint** and cannot replace either current part.
Face coupons are visible-face-down; base coupons are floor-down, mounts-up.

| STL | What it tests |
| --- | --- |
| [face_mount_fit_check_v2.stl](face_mount_fit_check_v2.stl) | Initial screen locating guides and blind mounting pilots, before empirical hole-position corrections. |
| [face_mount_fit_check_v3_m2.stl](face_mount_fit_check_v3_m2.stl) | Revision 3 face and M2 through-bolt recesses. |
| [face_mount_fit_check_v3_pilot.stl](face_mount_fit_check_v3_pilot.stl) | Revision 3 face with blind screen pilot holes. |
| [face_mount_fit_check_v4_m2.stl](face_mount_fit_check_v4_m2.stl) | Revision 4 corrected screen holes and M2 recesses. |
| [face_mount_fit_check_v4_pilot.stl](face_mount_fit_check_v4_pilot.stl) | Revision 4 face with blind screen pilot holes. |
| [face_mount_fit_check_v5_m2.stl](face_mount_fit_check_v5_m2.stl) | Final 18.85 mm screen spacing, alignment guides, M2 recesses and button holes. |
| [face_mount_fit_check_v6_m2.stl](face_mount_fit_check_v6_m2.stl) | Identical to the v5 face coupon. |
| [face_mount_fit_check_v7_m2.stl](face_mount_fit_check_v7_m2.stl) | Identical to the v5 face coupon. |
| [base_v5_fit_check.stl](base_v5_fit_check.stl) | Floor-only test of raised CAN and sensor mounts, without the screen face. |
| [base_v6_fit_check.stl](base_v6_fit_check.stl) | Floor-only test of the lowered CAN and sensor mounts. |
| [base_v7_fit_check.stl](base_v7_fit_check.stl) | Revision 6 interior layout with the leading floor edge trimmed for the v7 face plane. |

### Unversioned STL aliases

These names come from the older generator. **They do not select revision 8.**
The identities below describe the files currently shipped; rerunning a generator
can overwrite generic aliases, so use explicit versioned filenames when sharing.

| STL | Current contents |
| --- | --- |
| [chassis.stl](chassis.stl) | Identical to `chassis_v7_m2.stl`: the old one-piece L-chassis, floor down. |
| [cover.stl](cover.stl) | Identical to `cover_v7.stl`: compatible cover in assembly coordinates. |
| [cover_print.stl](cover_print.stl) | Identical to `cover_v7_print.stl`: compatible cover oriented for printing. |
| [face_fit_check.stl](face_fit_check.stl) | Identical to `face_mount_fit_check_v7_m2.stl`: a face coupon, **not** the current front with joining tabs. |

### Local design archives

The design workspace keeps the ZIP archives below as a record of earlier
revisions. This repository provides the printable STLs directly in the
[revision tables above](#stl-filename-guide).

| Archive | Design snapshot |
| --- | --- |
| `first-pass-before-display-mounts.zip` | Initial enclosure, cover and bare face trial before screen locating/mounting features. |
| `revision-2-before-stand-off-adjustment.zip` | First screen guides and mounting pilots, before the 0.50 mm correction. |
| `revision-3-before-final-hole-spacing.zip` | Revision 3 pilot/M2 alternatives before revision 4 hole corrections. |
| `revision-4-before-interior-mounts.zip` | Revision 4 screen fit before interior mounts and cover changes. |
| `revision-5-before-lowered-mounts.zip` | Raised interior mounts and original floor-only fit test. |
| `revision-6-before-face-down-edge-fix.zip` | Lowered mounts before the projecting floor edge was trimmed. |
| `revision-7-before-split-chassis.zip` | Final one-piece chassis before the front/floor split. |

## Front-to-floor hardware and assembly

Use **three M2×6 mm socket-head bolts**, three **4 mm across-flats nuts, 1.5 mm
thick**, and optional spring washers smaller than 4 mm outside diameter.
The modeled allowance is **2 mm total for each nut/washer stack**, including up
to 0.5 mm compressed washer thickness. Bolts clamp the pieces; they do not cut
threads into the plastic. The cover's existing fasteners are unchanged.

1. Dry-fit the front tabs onto the three floor pads. The U-shaped locating edges
   have 0.2 mm clearance per side. The horizontal bearing pads and existing face
   geometry establish the 5-degree recline. Clean any print burrs before assembly.
2. Drop a spring washer, if used, into each open hex pocket, followed by a nut.
   Do this before installing electronics that could obstruct access. The pockets
   prevent nut rotation during tightening; they are not press-fit retainers.
3. Insert the three bolts from underneath the floor and tighten to clamp the tabs.
   The nut can sit deeper when no washer is used. Both stack options are modeled.
4. Fit the screens using four M2×8 bolts and four nuts, then fit the buttons and
   other electronics. Install the cover using its four underside fasteners.
   The cover retains the earlier M3 flat-head screw provisions: Ø3.4 clearance
   holes and Ø2.6 pilots. Cover screw length remains a hardware fit check.

The bolt clearance holes are Ø2.2. Floor head recesses retain the proven screen
recess dimensions: **Ø4.2 × 2.1 deep**. Each joining pad is locally 4.1 mm thick,
leaving **2 mm of plastic beneath the head seat**. Nut pockets are 4.3 mm across
flats and 2 mm deep, leaving 1.7 mm of tab material beneath the nut. There are no
new visible front screws. Small ribs reinforce each tab at its root.

With a 6 mm under-head bolt, the tip is nominally 0.3 mm past a 2 mm stack, or
0.8 mm past the nut without a washer. Actual print and hardware fit still needs
the coupon check. Keep the previously fitted **M2×8** screen bolts for the screens;
the joining bolts have a different length because their clamping stack differs.

## CAN height and alignment correction

Revision 6 moved the lowered CAN cradle 3 mm inward to clear provisional front
button envelopes, but left the cable saddle in its earlier position. That left
the cradle and saddle centerlines offset by 3 mm. Revision 8 restores the raised
revision 5 arrangement:

- CAN PCB underside: **13.5 mm above the inside floor** (absolute Z=16).
- CAN lower-left position: X=38, Y=25; footprint 10.5 × 24.
- CAN cradle and cable saddle share centerline **X=43.25**.
- Cable saddle axis: **Z=22**, restored along with the raised cradle.
- Terminal wire entries face the rear; screws face up. The forward Dupont
  housings retain their 12 mm length allowance above the PCB.

Both cradle rails are generated from the same board position and height. The
rear CAN/reference-ground opening remains entirely in the removable cover.
The saddle holds the bundle on the floor. The loose ground reference still needs
its own electrical termination; its tie point is only mechanical retention.

## Retained dimensions and mounts

Exterior envelope remains **124 × 60 × 55.8 mm**, with 2.5 mm walls and 50.8 mm
clear internal height away from mounts. Coordinates: X right as viewed from the
front, Y toward the rear, Z upward from the outside floor. Inside floor is Z=2.5.

| Board | Footprint X × Y | Lower-left X, Y | Underside above inside floor |
| --- | --- | --- | --- |
| ESP32 | 23.75 × 18 | −58.5, 37.5 | 6.5 mm, unchanged for USB underneath |
| CAN | 10.5 × 24 | 38, 25 | 13.5 mm, raised again |
| Compass/magnetometer | 13 × 14 | −18, 36 | 1.5 mm |
| Gyro | 20.5 × 15.25 | 3, 31 | 1.5 mm |

The compass and gyro seats remain flat and coplanar at Z=4.0. The gyro has two
Ø3 PCB holes, 16.5 mm apart. Its printed Ø2 blind pilots have only **2.5 mm depth**;
use appropriately short mounting hardware. The earlier 6 mm gyro screws would
be too long for the lowered posts. PCB-to-floor clearance excludes local supports.

The front retains the physically fitted Ø33 viewing openings, locating guides,
Ø7 button holes, screen posts, Ø2.2 through holes and Ø4.2 × 2.1 head recesses.
Screen mounting spacing remains **18.85 mm**, 20.5 mm below each display center.
Display faces stay behind the apertures. No display geometry was repositioned.

The current ESP32 has USB underneath and header connections above. Its cradle,
left-wall USB opening, and plug clearance are unchanged. The future USB-up
orientation remains a separate unverified configuration. The cover uses the
same four fastening locations as revision 5: (−56,30), (56,30), (−25,54), (25,54).

## Source, previews and validation

The following filenames refer to the design workspace. The public checkout
contains this guide and the STL exports listed above.

- `parameters_v8.json`: current split design and hardware dimensions.
- `build_split_enclosure.py`: split geometry, exports, previews and validation.
- `build_enclosure.py`: shared shell/front geometry and checks; running it directly
  with `parameters.json` still generates the historical revision 7 L-piece.
- `interior_mounts.py`: board cradles, cable saddle and component envelopes.
- `validation_v8_split.json`: watertight/connected meshes, assembly clearances,
  cover lifting, nut/washer insertion, bolt clearances and bearing surfaces.
- `revision8_regression_checks.json`: cover compatibility, preserved front and
  sensor/controller mounts, and restored CAN alignment relative to earlier files.
- `split_v8_preview.png`, `front_v8_print_preview.png`, `floor_v8_print_preview.png`,
  `split_v8_exploded.png`, `joint_v8_detail.png`: saved visual references.
- `revisions/`: archived sources, measurements and outputs from earlier versions.

Checks use the measured PCB envelopes and provisional button, connector and plug
volumes. They do not establish actual cable routing, material strength, slicer
settings or physical fit. Verify the new joint before gluing boards. Dashboard
attachment and final sensor-axis alignment remain separate physical checks.

## Regenerate revision 8

With the design source files available, open their directory and install the
dependencies from `requirements.txt`. Run:

```sh
python build_split_enclosure.py
```

For a floor-only update:

```sh
python build_split_enclosure.py --floor-only
```

The latter validates the assembly but exports only `floor_v8_print.stl` and its
report. If joint dimensions change, rebuild both parts with the full command.
