; Export the symbols for: CarPallette, CarTiles, CloudsPalette, CloudsTiles, RoadPalette, RoadTiles, TitlePalette, TitleTiles

  .module img
  .area _TEXT

	.globl _idlerun_palette
    .globl _idlerun_palette_len
	.globl _idlerun_tiles
    .globl _idlerun_tiles_len

_idlerun_palette:
    .incbin "img/idlerun.ztp"
_idlerun_palette_len:
    .dw .-_idlerun_palette
_idlerun_tiles:
    .dw _idlerun_tiles_data
_idlerun_tiles_data:
    .incbin "img/idlerun.zts"
_idlerun_tiles_len:
    .dw .-_idlerun_tiles_data
