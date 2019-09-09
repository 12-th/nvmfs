SECTIONS
{
	kutest : {
		__start_kutest = .;
		*(kutest)
		__stop_kutest = .;
	}
}
