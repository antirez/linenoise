ranges = []
ARGF.each do |line|
    if line =~ /^(.*?)(?:\.\.(.*?))?;[FW]\s+# .*$/
        first = $1.to_i(16)
        last = $2 ? $2.to_i(16) : first
        if ranges.last && ranges.last[:l] + 1 == first
            ranges.last[:l] = last
        else
            ranges.push({ f: first, l: last })
        end
    end
end
ranges.each do |range|
    puts "{ #{'0x%X' % range[:f]}, #{'0x%X' % range[:l]} },"
end
