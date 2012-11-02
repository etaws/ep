include ./BuildDefs

OBJS_PATH       := $(PREFIX)/obj

LOCAL_PATH      := .
INCLUDES_PATH   := $(PREFIX)
SRCS_PATH       := $(PREFIX)

EXE             := ep_main

EP_SRCS        := ep_network.c
EP_OBJS        := $(patsubst %.c,$(OBJS_PATH)/%.o,$(EP_SRCS))
EP_DEPS        := $(patsubst %.c,$(OBJS_PATH)/%.d,$(EP_SRCS))

.PHONY: clean

all: $(EXE)

ep_main: ep_main.o $(EP_OBJS)
	$(LINK) $^ -o $@

$(OBJS_PATH)/%.o: $(SRCS_PATH)/%.c 
	@if test ! -d $(OBJS_PATH); then \
		/bin/mkdir -p $(OBJS_PATH); \
	fi; 
	$(call make-depend,$<,$@,$(subst .o,.d,$@))
	$(CC) $(CFLAGS) -I$(INCLUDES_PATH) -c $< -o $@

%.o: %.c
	$(call make-depend,$<,$@,$(subst .o,.d,$@))
	$(CC) $(CFLAGS) -I$(INCLUDES_PATH) -c $< -o $@

-include $(EP_DEPS)

clean:
	@$(RM) -rf $(OBJS_PATH) $(EXE) *.d *.o

define make-depend
        $(CC) -MM            \
         -MF $3         \
         -MP            \
         -MT $2         \
         $(CFLAGS)      \
         -I$(INCLUDES_PATH)      \
         $1
endef
