me=`basename $`

# Try git describe first, then VERSION.txt
version=`command -v git >/dev/null && git describe --match v[0-9]*.[0-9]** 2>/dev/null\
			| sed -e 's/v//' -e 's/\-g.*//' 2>/dev/null\
		|| cat VERSION.txt | sed 's/^[ \t]*//;s/[ \t]*$//'\
		|| echo "$me: Failed to get version info" && exit 1`
version_rc=`echo $version | sed -e 's/[-\.]/,/g'\
                                -e 's/^\([0-9]*,[0-9]*\)$/\1,0,0/'\
                                -e 's/^\([0-9]*,[0-9]*,[0-9]*\)$/\1,0/'`

echo \#define CRASHDETECT_VERSION \"$version\"
echo \#define CRASHDETECT_VERSION_RC $version_rc
