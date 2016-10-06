
OBJDIR=build
CFLAGS  = -g -Wall -Werror -MP -MMD -DCONFIG_DEBUG
LDFLAGS  = -g 

OBJS = core.o parser.o echoi.o main.o bus.o

$(OBJDIR)/%.o : ./%.c
	gcc -c $(CFLAGS) $(OPTFLAGS) -o $@ $< 

-include $(wildcard $(OBJDIR)/*.d)

objs = $(addprefix $(OBJDIR)/, $(OBJS))

all : dirs emul sockcon

sockcon: build/sockcon.o
	gcc $(LDLAGS) $(OPTFLAGS) -o $@ build/sockcon.o

emul : $(objs)
	gcc $(LDLAGS) $(OPTFLAGS) -o $@ $(objs)


.phony : clean dirs

dirs: 
	mkdir -p $(OBJDIR)

clean : 
	rm -f sockcon emul
	rm -rf $(OBJDIR)
