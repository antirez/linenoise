ARGF.each do |line|
    if line =~ /^(.*);.*;Mn;/
        puts "0x#{$1},"
    end
end
