echo '<html><body>'
for f in `ls $1`; do
    echo "<img src=\"$PWD/outputs/$f\"></img>"
done
echo '</body></html>'
