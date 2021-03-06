#!/bin/sh

echo "/* don't edit, auto generated by tools/build/genconfig.sh */"
echo "#ifndef __YOC_CONFIG_H__"
echo "#define __YOC_CONFIG_H__"
#echo ""

if [ -n "$2" ]; then
#define
cat $1 | sed 's/[[:space:]]//g' | grep "^[A-Z]" | grep -v "=n$" | sed 's/=y/ 1/' | sed 's/=/ /' | sed 's/^/#define /g' | awk '{printf "#ifndef %s\n%s\n#endif\n\n",$2,$0}'
# sed 's/\"/\\\"/g' | xargs -i echo -e "#define {}\n"
else
cat $1 | sed 's/[[:space:]]//g' | grep "^[A-Z]" | grep -v "=n$" | sed 's/=y/ 1/' | sed 's/=/ /' | sed 's/^/#define /g'
fi

#undef
cat $1 | sed 's/[[:space:]]//g' | grep "^[A-Z]" | grep "=n$" | sed 's/=n//' | sed 's/^/#undef /g'
#xargs -i echo "#undef {}"

#echo ""
echo "#endif"
