# Load the first layer of a TMX map and convert it to a C header file with an array of tile IDs.
import xml.etree.ElementTree as ET

def tmx_to_c_header(tmx_file, header_file):
    tree = ET.parse(tmx_file)
    root = tree.getroot()

    # Get the first layer
    layer = root.find('layer')
    data = layer.find('data').text.strip()

    # Convert the CSV data to a list of integers
    tile_ids = [int(tile_id) for tile_id in data.split(',')]

    # Write the C header file
    with open(header_file, 'w') as f:
        f.write('#ifndef MAP_DATA_H\n')
        f.write('#define MAP_DATA_H\n\n')
        f.write('#include <stdint.h>\n\n')
        f.write(f'#define MAP_WIDTH {layer.get("width")}\n')
        f.write(f'#define MAP_HEIGHT {layer.get("height")}\n\n')
        f.write('const uint8_t map_data[] = {\n')
        for i, tile_id in enumerate(tile_ids):
            f.write(f'    {tile_id-1},')
            if (i + 1) % int(layer.get('width')) == 0:
                f.write('\n')
        f.write('};\n\n')
        f.write('#endif // MAP_DATA_H\n')

if __name__ == '__main__':
    tmx_to_c_header('tutorial.tmx', 'tutorial_map.h')