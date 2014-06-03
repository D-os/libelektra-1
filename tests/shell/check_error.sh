@INCLUDE_COMMON@

echo
echo ELEKTRA CHECK ERROR
echo

check_version

FILE_SUFFIX=test_error

check_remaining_files $FILE_SUFFIX

ROOT=user/test/script
ROOT_FILE=${FILE_SUFFIX}_root.ecf
ROOT_MOUNTPOINT=/test/script
ROOT_MOUNTNAME=_test_script
if is_plugin_available dump
then
	$KDB mount $ROOT_FILE $ROOT_MOUNTPOINT dump > /dev/null 2>&1
	succeed_if "could not mount root: $ROOT_FILE at $ROOT_MOUNTPOINT"

	$KDB set $ROOT/valueable_data important_unrecoverable_data > /dev/null
	succeed_if "cannot set valueable data"

	$KDB setmeta $ROOT/valueable_data trigger/error 10
	succeed_if "cannot set meta data"
fi

TMPFILE=`mktemp`
ERROR_FILE=${FILE_SUFFIX}_error.ecf
ERROR_MOUNTPOINT=/test/script/error
ERROR_MOUNTNAME=_test_script_error
if is_plugin_available error
then
	echo "Testing operations on errornous backends"

	$KDB mount $ERROR_FILE $ERROR_MOUNTPOINT dump error > /dev/null 2>&1
	succeed_if "could not mount error at $ERROR_MOUNTPOINT"

	$KDB mv $ROOT/valueable_data $ROOT/error/dump > $TMPFILE 2>&1
	[ $? -ne 0 ]
	succeed_if "Was able to move to error plugin"

	grep "Error (#10) occurred!" $TMPFILE > /dev/null
	succeed_if "Triggered error did not occur"

	grep "Reason: from error plugin" $TMPFILE > /dev/null
	succeed_if "Error does not stem from error plugin"

	[ "x`$KDB ls $ROOT 2> /dev/null`" = "x$ROOT/valueable_data" ]
	succeed_if "cant ls $ROOT (may mean that $ROOT folder is not clean)"

	[ "x`$KDB get $ROOT/valueable_data 2> /dev/null`" = "ximportant_unrecoverable_data" ]
	succeed_if "Important data lost"


	$KDB cp $ROOT/valueable_data $ROOT/error/dump > $TMPFILE 2>&1
	[ $? -ne 0 ]
	succeed_if "Was able to copy to error plugin"

	grep "Error (#10) occurred!" $TMPFILE > /dev/null
	succeed_if "Triggered error did not occur"

	grep "Reason: from error plugin" $TMPFILE > /dev/null
	succeed_if "Error does not stem from error plugin"

	[ "x`$KDB ls $ROOT 2> /dev/null`" = "x$ROOT/valueable_data" ]
	succeed_if "cant ls $ROOT (may mean that $ROOT folder is not clean)"

	[ "x`$KDB get $ROOT/valueable_data 2> /dev/null`" = "ximportant_unrecoverable_data" ]
	succeed_if "Important data lost"
fi
rm $TMPFILE



if is_plugin_available error
then
	$KDB umount $ERROR_MOUNTNAME >/dev/null
	succeed_if "could not umount $ERROR_MOUNTNAME"
fi

if is_plugin_available dump
then
	$KDB umount $ROOT_MOUNTNAME >/dev/null
	succeed_if "could not umount $ROOT_MOUNTNAME"
fi

rm -f $USER_FOLDER/$FILE_SUFFIX*
rm -f $SYSTEM_FOLDER/$FILE_SUFFIX*

end_script error