echo '<html><body>'
for f in `ls outputs/`; do
    echo "<img src=\"$PWD/outputs/$f\"></img>"
done
echo '</body></html>'
