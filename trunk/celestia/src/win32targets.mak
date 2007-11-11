$(OUTDIR)\$(TARGETLIB) : $(OUTDIR) $(OBJS)
	lib /out:$(OUTDIR)\$(TARGETLIB) $(OBJS)

"$(OUTDIR)" :
	if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

clean:
	-@del $(OUTDIR)\$(TARGETLIB) $(OBJS) $(SCRIPTOBJECS) $(SPICEOBJS)

