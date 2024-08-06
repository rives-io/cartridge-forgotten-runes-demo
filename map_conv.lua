local map = require 'map'
io.write('{\n')
for l,layer in ipairs(map.layers) do
  io.write('    {\n')
  for y=0,map.height-1 do
    io.write('        {')
    for x=0,map.width-1 do
      io.write(layer.data[y*map.height+x+1]-1,',')
    end
    io.write('},\n')
  end
  io.write('    },\n')
end
io.write('}\n')
