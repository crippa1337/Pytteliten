# Delete old version
if [ -f "./pytteliten-mini" ]; then
    rm ./pytteliten-mini
fi

# Minify the code
python3 minifier.py

# Compress the source copy
echo Finding best compression parameters...
SMALLEST=1000000
LAUNCHER_SIZE=$(stat -c%s "launcher.sh")
for MF in hc3 hc4 bt2 bt3 bt4
do
    for NICE in {4..273}
    do
        lzma -f -k --lzma1=preset=9,lc=0,lp=0,pb=0,mf=$MF,nice=$NICE pytteliten-mini.cpp
        FILESIZE=$(stat -c%s "pytteliten-mini.cpp.lzma")
        if [ "$FILESIZE" -lt "$SMALLEST" ]; then
            echo mf=$MF nice=$NICE size=$(($LAUNCHER_SIZE + $FILESIZE))
            cp -f pytteliten-mini.cpp.lzma pytteliten-mini-smallest.cpp.lzma
            SMALLEST=$FILESIZE
        fi
    done
done

# Create build script
cat launcher.sh pytteliten-mini-smallest.cpp.lzma > ./pytteliten-mini

# Delete compressed sources
rm -f pytteliten-mini.cpp.lzma
rm -f pytteliten-mini-smallest.cpp.lzma

# Make script executable
chmod +x ./pytteliten-mini

# Print pytteliten-mini file size
echo
echo pytteliten-mini size: $(($LAUNCHER_SIZE + $SMALLEST)) bytes