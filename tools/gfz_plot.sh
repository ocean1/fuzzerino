#!/bin/bash

if  [[ $# -lt 1 ]] ; then
   echo "usage: $0 <gfz plot data> [-u]" >&2; exit 1
fi

script=$(
cat <<_EOF_
set terminal png truecolor enhanced size 1000,300 butt
set xdata time
set timefmt '%s'
set format x "%b %d\n%H:%M"
set tics font 'small'
unset mxtics
unset mytics
set grid xtics linetype 0 linecolor rgb '#e0e0e0'
set grid ytics linetype 0 linecolor rgb '#e0e0e0'
set border linecolor rgb '#50c0f0'
set tics textcolor rgb '#000000'
set key outside
set autoscale xfixmin
set autoscale xfixmax
_EOF_
)

if [[ "$2" == "-u" ]] ; then

    (cat <<_EOF_
$script
set output '$1_unique.png'
plot '$1' using 1:3 with lines title 'unique' linecolor rgb '#0090ff' linewidth 3, \\
     '' using 1:4 with lines title 'coverage' linecolor rgb '#009900' linewidth 3, \\
     '' using 1:9 with lines title 'havoc' linecolor rgb '#ff99ff' linewidth 3
_EOF_
) | gnuplot

else

    (cat <<_EOF_
$script
set output '$1_full.png'
plot '$1' using 1:2 with lines title 'generated' linecolor rgb '#000000' linewidth 3, \\
     '' using 1:3 with lines title 'unique' linecolor rgb '#0090ff' linewidth 3, \\
     '' using 1:4 with lines title 'coverage' linecolor rgb '#009900' linewidth 3, \\
     '' using 1:5 with lines title 'execs' linecolor rgb '#c00080' linewidth 3, \\
     '' using 1:6 with lines title 'crashes' linecolor rgb '#c000f0' linewidth 3, \\
     '' using 1:7 with lines title 'tmouts' linecolor rgb '#ff8000' linewidth 3, \\
     '' using 1:8 with lines title 'execs/sec' linecolor rgb '#5bdaff' linewidth 3, \\
     '' using 1:9 with lines title 'havoc' linecolor rgb '#ff99ff' linewidth 3
_EOF_
) | gnuplot

fi