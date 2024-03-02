local concat = function(...)
  local res = ""
  for str in ... do
    res = res .. str
  end
end

print(concat("hello,", "", "world"))
