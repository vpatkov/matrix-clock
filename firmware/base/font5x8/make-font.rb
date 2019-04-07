#!/usr/bin/ruby -w

if ARGV.size != 2
    abort "Usage: #$0 <ascii-file> <output.hpp>"
end

ascii_file, output_file = ARGV
font_dir = File.dirname(ascii_file)

ascii_map = Array.new(128) {0}
font_bitmaps = Array.new(1)

def pbm_to_bitmap(file)
    pbm = File.readlines(file)
        .map{ |s| s.gsub(/#.*/, '') }
        .join(' ').split
    if pbm[0] != 'P1'
        abort "#{file}: not a Plain PBM file"
    elsif pbm[1] != '5' || pbm[2] != '8'
        abort "#{file}: not a 5x8 bitmap"
    else
        return pbm.drop(3).join.chars.take(40)
            .each_slice(5).to_a.transpose
            .map{ |xs| xs.join.to_i(2) }
    end
end

class Array
    def push_uniq(x)
        self.index(x) or self.push(x).size - 1
    end
end

File.open(ascii_file, mode: 'r').each.with_index(1) do |line, lineno|
    cols = line.gsub(/#.*/, '').split
    if cols.size == 0
        next
    elsif cols.size != 2
        abort "Invalid line #{lineno}: #{line}"
    end
    char = cols[0]
    bitmap = pbm_to_bitmap(File.expand_path(cols[1], font_dir))
    if char.size == 1
        ascii_map[char.ord] = font_bitmaps.push_uniq(bitmap)
    elsif char.match?(/^0x\h\h$/)
        ascii_map[char.to_i(16)] = font_bitmaps.push_uniq(bitmap)
    elsif char == 'default'
        font_bitmaps[0] = bitmap
    else
        abort "Invalid character at line #{lineno}: #{char}"
    end
end

if font_bitmaps[0].nil?
    abort "Default character must be defined"
end

File.open(output_file, mode: 'w') do |file|
    file.write("/* Auto-generated file. Do not edit. */\n\n")

    file.write("#ifndef FONT5X8_HPP_\n")
    file.write("#define FONT5X8_HPP_\n\n")

    file.write("#include <avr/pgmspace.h>\n\n")

    file.write("/* Bitmaps for the 5x8 font */\n")
    file.write("constexpr uint8_t font5x8[][5] PROGMEM = {\n")
    font_bitmaps.each do |bitmap|
        file.write("\t{#{bitmap.join(', ')}},\n")
    end
    file.write("};\n\n")

    file.write("/* ASCII -> font5x8 */\n")
    file.write("constexpr uint8_t ascii[128] PROGMEM = {\n")
    ascii_map.each_slice(8) do |arr|
        file.write("\t#{arr.join(', ')},\n")
    end
    file.write("};\n\n")

    file.write("#endif\n")
end
