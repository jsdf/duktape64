
// #include <assert.h>
#include <nusys.h>

#include "graphic.h"
#include "main.h"
#include "n64logo.h"
#include "stage00.h"

#include "ed64io.h"
#include "ed64io_sys.h"

#include "duktape.h"

#include "duk_console.h"

#define PRINTF ed64PrintfSync2
#define DEBUGPRINT 0
#if DEBUGPRINT
#define DBGPRINT ed64PrintfSync2
#else
#define DBGPRINT(args...)
#endif

Vec3d cameraPos = {-200.0f, -200.0f, -200.0f};
Vec3d cameraTarget = {0.0f, 0.0f, 0.0f};
Vec3d cameraUp = {0.0f, 1.0f, 0.0f};

// the positions of the squares we're gonna draw
#define NUM_SQUARES 5
struct Vec3d squares[NUM_SQUARES] = {
    {0.0f, 0.0f, 0.0f},   {0.0f, 0.0f, 0.0f},    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 200.0f}, {0.0f, 0.0f, -100.0f},
};

int squaresRotations[NUM_SQUARES];
int initSquaresRotations[NUM_SQUARES] = {
    0, 20, 40, 40, 40,
};

// this is a boolean but the older version of C used by the N64 compiler
// (roughly C89), doesn't have a bool type, so we just use integers
int squaresRotationDirection;

int showN64Logo;

static duk_context* ctx;

static int multipartMessageOngoing = FALSE;

static void my_fatal(void* udata, const char* msg) {
  DUK_USE_FATAL_HANDLER(udata, msg);
}

static void n64log(const char* msg) {
  nuDebConPrintf(0, "%s", msg);
}

#define PRINTF_FLUSH()                   \
  while (ed64AsyncLoggerFlush() != -1) { \
    evd_sleep(1);                        \
  }

// the 'setup' function
void initStage00() {
  // the advantage of initializing these values here, rather than statically, is
  // that if you switch stages/levels, and later return to this stage, you can
  // call this function to reset these values.
  squaresRotationDirection = FALSE;
  showN64Logo = FALSE;

  // In the older version of C used by the N64 compiler (roughly C89), variables
  // must be declared at the top of a function or block scope. This is an
  // example of using block scope to declare a variable in the middle of a
  // function.
  {
    int i;
    for (i = 0; i < NUM_SQUARES; ++i) {
      squaresRotations[i] = initSquaresRotations[i];
    }
  }

  ctx = duk_create_heap(NULL, NULL, NULL, NULL, my_fatal);
  ed64PrintfSync2("created JS heap at %x\n", ctx);
  duk_console_init(ctx, 0);

  nuDebConClear(0);
  nuDebConWindowPos(0, 4, 4);
}

char* js_wrap_pre = "(function() { return ";
char* js_wrap_post = " })()";
void js_eval(char* jsText) {
  int success;
  char* wrapped;

  wrapped = (char*)malloc(strlen(js_wrap_pre) + strlen(jsText) +
                          strlen(js_wrap_post));
  if (!wrapped) {
    PRINTF("js_eval failed to alloc memory\n");
    return;
  }
  *wrapped = '\0';
  strcat(wrapped, js_wrap_pre);
  strcat(wrapped, jsText);
  strcat(wrapped, js_wrap_post);

  DBGPRINT("evaling: %s\n", wrapped);
  duk_push_string(ctx, wrapped);
  success = (duk_peval(ctx) == 0);
  PRINTF_FLUSH();
  ed64Printf("$EVAL$START\n");
  if (success) {
    duk__fmt(ctx);
    nuDebConPrintf(0, "> %s\n", jsText);
    nuDebConPrintf(0, "=> %s\n", duk__console_last_result);
  } else {
    ed64Printf("%s\n", duk_safe_to_string(ctx, -1));
  }
  PRINTF_FLUSH();
  ed64Printf("$EVAL$END\n");
  PRINTF_FLUSH();
  duk_pop(ctx);
  free(wrapped);
}

void js_exec(char* jsText) {
  int success;
  DBGPRINT("execing: %s\n", jsText);
  duk_push_string(ctx, jsText);
  success = (duk_peval(ctx) == 0);
  if (!success) {
    // exec failure
    printf("%s\n", duk_safe_to_string(ctx, -1));
  }
  duk_pop(ctx);
}

#define USB_BUFFER_SIZE 128

int evalBufferSize = 0;  // in bytes
char* evalBuffer = NULL;

#define EVAL_BODY_MAX 504

#define FLAG_HAS_MORE_CHUNKS 1

int ed64UsbCmdListener() {
  char cmd;
  u16 bodySize;
  u16 flags;
  const char* body;
  u32 usb_rx_buff32[USB_BUFFER_SIZE];
  char* usb_rx_buff8 = (char*)usb_rx_buff32;
  memset(usb_rx_buff8, 0, USB_BUFFER_SIZE * 4);

  if (evd_fifoRxf())  // when pin low, receive buffer not empty yet
    return FALSE;

  DBGPRINT("starting read\n");
  // returns timeout error, at which time we just try again
  while (evd_fifoRd(usb_rx_buff32, 1)) {
    DBGPRINT("sleeping\n");
    evd_sleep(100);
  }
  DBGPRINT("dma read done\n");

  DBGPRINT("message: %c%c%c%c\n", usb_rx_buff8[0], usb_rx_buff8[1],
           usb_rx_buff8[2], usb_rx_buff8[3]);

  if (usb_rx_buff8[0] != 'C' || usb_rx_buff8[1] != 'M' ||
      usb_rx_buff8[2] != 'D') {
    PRINTF("invalid message\n");
    return FALSE;
  }

  cmd = usb_rx_buff8[3];
  DBGPRINT("got command: '%c'\n", cmd);

  // parse packet
  bodySize = *((u16*)(usb_rx_buff8 + 4));  // start + 4 bytes
  flags = *((u16*)(usb_rx_buff8 + 6));     // start + 6 bytes
  body = usb_rx_buff8 + 8;                 // start + 8 bytes
  DBGPRINT("bodySize=%d flags=%d\n", bodySize, flags);

  switch (cmd) {
    case 's':  // signal
    {
      int success;
      ed64PrintfSync2("about to eval\n");
      // duk_push_string(ctx, "console.log(JSON.stringify({a:{b:1}}))");
      duk_push_string(
          ctx,
          "const fanout = 2;"
          "function doStuff(maxDepth, depth) {"
          "  const a = {};"
          "  for (var i = 0; i < fanout; i++) {"
          "    a[i] = depth == maxDepth ? {} : doStuff(maxDepth, depth + 1);"
          "  }"
          "  return a;"
          "}\n"
          "console.log(doStuff(2, 0));");
      success = (duk_peval(ctx) == 0);
      PRINTF_FLUSH()
      if (success) {
        ed64PrintfSync2("success\n");
        // duk__fmt(ctx);
      } else {
        ed64PrintfSync2("failure\n");
        ed64Printf("%s\n", duk_safe_to_string(ctx, -1));
      }
      PRINTF_FLUSH()
      duk_pop(ctx);

      return TRUE;
    }
    case 'e':  // eval
    case 'x':  // exec
    {
      char* evalBufferNext;
      int isLastChunk = !(flags & FLAG_HAS_MORE_CHUNKS);

      if (bodySize > EVAL_BODY_MAX || bodySize < 1) {
        PRINTF("message bodySize field invalid %d\n", bodySize);
        return TRUE;
      }
      bodySize = MIN(bodySize, EVAL_BODY_MAX);

      // buffer for next js string (appended to current)
      DBGPRINT("will alloc: %d\n", evalBufferSize + bodySize);
      evalBufferNext = (char*)malloc(evalBufferSize + bodySize);
      if (!evalBufferNext) {
        PRINTF("ed64UsbCmdListener failed to alloc memory\n");
        return TRUE;
      }

      if (evalBuffer) {
        // copy any existing stuff into the new buffer
        memcpy(evalBufferNext, evalBuffer, evalBufferSize);
      }
      memcpy(evalBufferNext + evalBufferSize, body, bodySize);
      free(evalBuffer);
      evalBuffer = evalBufferNext;
      evalBufferSize = evalBufferSize + bodySize;

      if (isLastChunk) {
        // we're ready to eval the text
        // make sure we have a '\0' in there somewhere
        int hasNullTerminator = FALSE;
        {
          int i;
          for (i = 0; i < evalBufferSize; ++i) {
            if (evalBuffer[i] == '\0') {
              hasNullTerminator = TRUE;
              break;
            }
          }
        }
        if (!hasNullTerminator) {
          PRINTF("has no null terminator\n");
          return TRUE;
        }

        // in eval case we return output, in exec case we don't
        if (cmd == 'e') {
          js_eval(evalBuffer);
        } else {
          js_exec(evalBuffer);
        }

        // dispose of buffer
        free(evalBuffer);
        evalBuffer = NULL;
        evalBufferSize = 0;
        multipartMessageOngoing = FALSE;
      } else {
        // multi-part message, wait for more chunks
        DBGPRINT("multi-part message\n");
        multipartMessageOngoing = TRUE;
      }

      return TRUE;
    }
    default:
      PRINTF("invalid command: '%c'\n", cmd);
  }

  return FALSE;
}

// the 'update' function
int gameTick = 0;
void updateGame00() {
  gameTick++;

  // read controller input from controller 1 (index 0)
  nuContDataGetEx(contdata, 0);
  // We check if the 'A' Button was pressed using a bitwise AND with
  // contdata[0].trigger and the A_BUTTON constant.
  // The contdata[0].trigger property is set only for the frame that the
  // button is initially pressed. The contdata[0].button property is similar,
  // but stays on for the duration of the button press.
  if (contdata[0].trigger & A_BUTTON) {
    // when A button is pressed, reverse rotation direction
    squaresRotationDirection = !squaresRotationDirection;
  }

  if (contdata[0].button & B_BUTTON) {
    // when B button is held, change squares into n64 logos
    showN64Logo = TRUE;
  } else {
    showN64Logo = FALSE;
  }

  // update square rotations
  {
    int i;
    for (i = 0; i < NUM_SQUARES; ++i) {
      squaresRotations[i] =
          (squaresRotations[i] + (squaresRotationDirection ? 1 : -1)) % 360;
    }
  }

  // poll for command from client
  ed64UsbCmdListener();
  while (multipartMessageOngoing) {
    while (!ed64UsbCmdListener() && multipartMessageOngoing) {
      evd_sleep(100);
    }
    evd_sleep(100);
  }
  // send buffered output
  ed64AsyncLoggerFlush();
}

// the 'draw' function
void makeDL00() {
  unsigned short perspNorm;
  GraphicsTask* gfxTask;

  // switch the current graphics task
  // also updates the displayListPtr global variable
  gfxTask = gfxSwitchTask();

  // prepare the RCP for rendering a graphics task
  gfxRCPInit();

  // clear the color framebuffer and Z-buffer, similar to glClear()
  gfxClearCfb();

  // initialize the projection matrix, similar to glPerspective() or
  // glm::perspective()
  guPerspective(&gfxTask->projection, &perspNorm, FOVY, ASPECT, NEAR_PLANE,
                FAR_PLANE, 1.0);

  // Our first actual displaylist command. This writes the command as a value
  // at the tail of the current display list, and we increment the display
  // list tail pointer, ready for the next command to be written. As for what
  // this command does... it's just required when using a perspective
  // projection. Try pasting 'gSPPerspNormalize' into google if you want more
  // explanation, as all the SDK documentation has been placed online by
  // hobbyists and is well indexed.
  gSPPerspNormalize(displayListPtr++, perspNorm);

  // initialize the modelview matrix, similar to gluLookAt() or glm::lookAt()
  guLookAt(&gfxTask->modelview, cameraPos.x, cameraPos.y, cameraPos.z,
           cameraTarget.x, cameraTarget.y, cameraTarget.z, cameraUp.x,
           cameraUp.y, cameraUp.z);

  // load the projection matrix into the matrix stack.
  // given the combination of G_MTX_flags we provide, effectively this means
  // "replace the projection matrix with this new matrix"
  gSPMatrix(displayListPtr++,
            // we use the OS_K0_TO_PHYSICAL macro to convert the pointer to
            // this matrix into a 'physical' address as required by the RCP
            OS_K0_TO_PHYSICAL(&(gfxTask->projection)),
            // these flags tell the graphics microcode what to do with this
            // matrix documented here:
            // http://n64devkit.square7.ch/tutorial/graphics/1/1_3.htm
            G_MTX_PROJECTION |  // using the projection matrix stack...
                G_MTX_LOAD |    // don't multiply matrix by previously-top
                                // matrix in stack
                G_MTX_NOPUSH    // don't push another matrix onto the stack
                                // before operation
  );

  gSPMatrix(displayListPtr++, OS_K0_TO_PHYSICAL(&(gfxTask->modelview)),
            // similarly this combination means "replace the modelview matrix
            // with this new matrix"
            G_MTX_MODELVIEW | G_MTX_NOPUSH | G_MTX_LOAD);

  {
    int i;
    Vec3d* square;
    for (i = 0; i < NUM_SQUARES; ++i) {
      square = &squares[i];
      // create a transformation matrix representing the position of the
      // square
      guPosition(&gfxTask->objectTransforms[i],
                 // rotation
                 squaresRotations[i],  // roll
                 0.0f,                 // pitch
                 0.0f,                 // heading
                 1.0f,                 // scale
                 // position
                 square->x, square->y, square->z);

      // push relative transformation matrix
      gSPMatrix(displayListPtr++,
                OS_K0_TO_PHYSICAL(&(gfxTask->objectTransforms[i])),
                G_MTX_MODELVIEW |  // operating on the modelview matrix stack...
                    G_MTX_PUSH |   // ...push another matrix onto the stack...
                    G_MTX_MUL  // ...which is multipled by previously-top matrix
                               // (eg. a relative transformation)
      );

      if (showN64Logo) {
        drawN64Logo();
      } else {
        drawSquare();
      }

      // pop the matrix that we added back off the stack, to move the drawing
      // position back to where it was before we rendered this object
      gSPPopMatrix(displayListPtr++, G_MTX_MODELVIEW);
    }
  }

  // mark the end of the display list
  gDPFullSync(displayListPtr++);
  gSPEndDisplayList(displayListPtr++);

  // assert that the display list isn't longer than the memory allocated for
  // it, otherwise we would have corrupted memory when writing it. isn't
  // unsafe memory access fun? this could be made safer by instead asserting
  // on the displaylist length every time the pointer is advanced, but that
  // would add some overhead. assert(displayListPtr - gfxTask->displayList <
  // MAX_DISPLAY_LIST_COMMANDS);

  // create a graphics task to render this displaylist and send it to the RCP
  nuGfxTaskStart(
      gfxTask->displayList,
      (int)(displayListPtr - gfxTask->displayList) * sizeof(Gfx),
      NU_GFX_UCODE_F3DEX,  // load the 'F3DEX' version graphics microcode,
                           // which runs on the RCP to process this display
                           // list
      NU_SC_NOSWAPBUFFER
      // NU_SC_SWAPBUFFER     // tells NuSystem to immediately display the
      // frame on screen after the RCP finishes rendering it
  );

  nuDebConDisp(NU_SC_SWAPBUFFER);
}

// A static array of model vertex data.
// This include the position (x,y,z), texture U,V coords (called S,T in the
// SDK) and vertex color values in r,g,b,a form. As this data will be read by
// the RCP via direct memory access, which is required to be 16-byte aligned,
// it's a good idea to annotate it with the GCC attribute
// `__attribute__((aligned (16)))`, to force it to be 16-byte aligned.
Vtx squareVerts[] __attribute__((aligned(16))) = {
    //  x,   y,  z, flag, S, T,    r,    g,    b,    a
    {-64, 64, -5, 0, 0, 0, 0x00, 0xff, 0x00, 0xff},
    {64, 64, -5, 0, 0, 0, 0x00, 0x00, 0x00, 0xff},
    {64, -64, -5, 0, 0, 0, 0x00, 0x00, 0xff, 0xff},
    {-64, -64, -5, 0, 0, 0, 0xff, 0x00, 0x00, 0xff},
};

void drawSquare() {
  // load vertex data for the triangles
  gSPVertex(displayListPtr++, &(squareVerts[0]), 4, 0);
  // depending on which graphical features, the RDP might need to spend 1 or 2
  // cycles to render a primitive, and we need to tell it which to do
  gDPSetCycleType(displayListPtr++, G_CYC_1CYCLE);
  // use antialiasing, rendering an opaque surface
  gDPSetRenderMode(displayListPtr++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
  // reset any rendering flags set when drawing the previous primitive
  gSPClearGeometryMode(displayListPtr++, 0xFFFFFFFF);
  // enable smooth (gourad) shading and z-buffering
  gSPSetGeometryMode(displayListPtr++, G_SHADE | G_SHADING_SMOOTH | G_ZBUFFER);

  // actually draw the triangles, using the specified vertices
  gSP2Triangles(displayListPtr++, 0, 1, 2, 0, 0, 2, 3, 0);

  // Mark that we've finished sending commands for this particular primitive.
  // This is needed to prevent race conditions inside the rendering hardware
  // in the case that subsequent commands change rendering settings.
  gDPPipeSync(displayListPtr++);
}

// this is an example of rendering a model defined as a set of static display
// lists
void drawN64Logo() {
  gDPSetCycleType(displayListPtr++, G_CYC_1CYCLE);
  gDPSetRenderMode(displayListPtr++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
  gSPClearGeometryMode(displayListPtr++, 0xFFFFFFFF);
  gSPSetGeometryMode(displayListPtr++, G_SHADE | G_SHADING_SMOOTH | G_ZBUFFER);

  // The gSPDisplayList command causes the RCP to render a static display
  // list, then return to this display list afterwards. These 4 display lists
  // are defined in n64logo.h, and were generated from a 3D model using a
  // conversion script.
  gSPDisplayList(displayListPtr++, N64Yellow_PolyList);
  gSPDisplayList(displayListPtr++, N64Red_PolyList);
  gSPDisplayList(displayListPtr++, N64Blue_PolyList);
  gSPDisplayList(displayListPtr++, N64Green_PolyList);

  gDPPipeSync(displayListPtr++);
}

// the nusystem callback for the stage, called once per frame
void stage00(int pendingGfx) {
  // produce a new displaylist (unless we're running behind, meaning we
  // already have the maximum queued up)
  if (pendingGfx < 1)
    makeDL00();

  // update the state of the world for the next frame
  updateGame00();
}
