// terrain.c
// Terrain generation.

#include <math.h>

#include "blocks.h"
#include "world.h"
#include "noise.h"
#include "trees.h"
#include "terrain.h"

/***********
 * Globals *
 ***********/

float TR_NOISE_OFFSET = 7300;

/*************
 * Functions *
 *************/

block terrain_block(region_pos pos) {
  static int xcache = 3, ycache = 7;
  static int terrain = 0;
  static int dirt = 1;
  static int sandy = 0;
  static int cave_layer_1_b = 0, cave_layer_1_t = 0;
  static int cave_layer_2_b = 0, cave_layer_2_t = 0;
  static int cave_layer_3_b = 0, cave_layer_3_t = 0;
  static float nlst = 0, nlow = 0, nmid = 0, nhig = 0, nhst = 0;
  static float depths = 0, oceans = 0, plains = 0, hills = 0, mountains = 0;
  int tunnel = 0;
  int altitude = 0;
  if (xcache != pos.x || ycache != pos.y) {
    xcache = pos.x; ycache = pos.y;
    // recompute everything:
    // generate some noise at each frequency (which we'll reuse several times):
    get_noise(pos.x, pos.y, &nlst, &nlow, &nmid, &nhig, &nhst);
    // compute geoform mixing factors:
    depths = 0;
    oceans = 0;
    plains = 0;
    hills = 0;
    mountains = 0;
    compute_geoforms(nlst, &depths, &oceans, &plains, &hills, &mountains);
    // compute terrain height:
    terrain = get_terrain_height(
      nlst, nlow, nmid, nhig, nhst,
      depths, oceans, plains, hills, mountains
    );
    // compute cave layers:
    get_cave_layers(
      nlst, nlow, nmid, nhig, nhst,
      depths, oceans, plains, hills, mountains,
      &cave_layer_1_b, &cave_layer_1_t,
      &cave_layer_2_b, &cave_layer_2_t,
      &cave_layer_3_b, &cave_layer_3_t
    );
    // dirt depth:
    dirt = TR_DIRT_MID + (int) (
      nmid * TR_DIRT_VAR
    );
    // compute a tree milieu:
    compute_tree_milieu(pos.x, pos.y, &TREE_MILIEU);
    // sandiness:
    sandy =
      oceans * oceans > (TR_BEACH_THRESHOLD + (0.03 * terrain - TR_SEA_LEVEL));
  }
  // Altitude measures height above/below the base terrain height:
  altitude = pos.z - terrain;
  // DEBUG: (tunnels are expensive)
  tunnel = 0;
  /*
  // compute tunnel value:
  tunnel = get_tunnel(
    &pos,
    depths, oceans, plains, hills, mountains // TODO: Use these arguments!
  );
  // */
  if (
    tunnel
  &&
    altitude <= 0
  &&
    (
      terrain > TR_SEA_LEVEL
    ||
      altitude < -TR_TUNNEL_UNDERSEA_OFFSET
    )
  ) {
    return B_AIR;
  }
  if (altitude == 0) {
    if (sandy) {
      return B_SAND;
    } else if (pos.z >= TR_SEA_LEVEL) {
      return B_GRASS;
    } else {
      return B_DIRT;
    }
  } else if (altitude > 0) {
    if (pos.z > TR_SEA_LEVEL) {
      if (altitude <= TREE_MAX_CANOPY_HEIGHT) {
        return tree_block(pos, &TREE_MILIEU);
      }
      return B_AIR;
    } else {
      return B_WATER;
    }
  } else {
    if (altitude > -dirt) {
      if (sandy) {
        return B_SAND;
      } else {
        return B_DIRT;
      }
    }
    return B_STONE;
  }
}

void get_geoforms(
  int x, int y,
  float *depths, float *oceans, float *plains, float *hills, float *mountains
) {
  float noise = 0, ignore = 0;
  get_noise(x, y, &noise, &ignore, &ignore, &ignore, &ignore);
  compute_geoforms(noise, depths, oceans, plains, hills, mountains);
}
