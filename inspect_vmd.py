import struct
import sys

def read_vmd(filepath):
    with open(filepath, 'rb') as f:
        header = f.read(30)
        model_name = f.read(20)
        count_data = f.read(4)
        count = struct.unpack('<I', count_data)[0]
        
        print(f"Bone Keyframes: {count}")
        
        found_count = 0
        for i in range(count):
            data = f.read(111)
            name_bytes = data[0:15]
            try:
                name = name_bytes.split(b'\x00')[0].decode('shift-jis')
            except:
                name = str(name_bytes)
                
            if b'\x49\x4b' in name_bytes:
                print(f"Found Half-width IK: {name_bytes.hex()}")
            if "足IK" in name or "足ＩＫ" in name:
                # print(f"Name Hex: {name_bytes.hex()}")
                frame = struct.unpack('<I', data[15:19])[0]
                pos = struct.unpack('<fff', data[19:31])
                print(f"Bone: {name}, Frame: {frame}, Pos: {pos}")
                found_count += 1
                if found_count > 20: return

read_vmd('motions/TDA.vmd')
