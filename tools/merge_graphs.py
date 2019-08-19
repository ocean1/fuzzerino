import os, sys, shutil

directory = sys.argv[1]
final_html = ""

for d in os.listdir(directory):
    subdir = os.path.join(directory, d)
    for file in os.listdir(subdir):
        if file.endswith('.png'):
            os.rename(os.path.join(subdir, file), os.path.join(directory, d + '_' + file))
        elif file.endswith('.html'):
            contents = open(os.path.join(subdir, file), 'r').read()
            contents = contents.replace('src="', 'src="{}_'.format(d))
            final_html += contents

    shutil.rmtree(subdir, ignore_errors=True)

with open(os.path.join(directory, 'index.html'), 'w') as f:
   f.write(final_html)