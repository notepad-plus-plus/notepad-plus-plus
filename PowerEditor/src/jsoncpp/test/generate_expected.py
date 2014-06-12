import glob
import os.path
for path in glob.glob( '*.json' ):
    text = file(path,'rt').read()
    target = os.path.splitext(path)[0] + '.expected'
    if os.path.exists( target ):
        print 'skipping:', target
    else:
        print 'creating:', target
        file(target,'wt').write(text)

