# removes all files created during testing
import glob
import os

paths = []
for pattern in [ '*.actual', '*.actual-rewrite', '*.rewrite', '*.process-output' ]:
    paths += glob.glob( 'data/' + pattern )

for path in paths:
    os.unlink( path )
