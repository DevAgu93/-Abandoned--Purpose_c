inline void
openg_push_quad(game_renderer *renderer,
            render_vertex *vertices,
            u32 vertexArrayOffset)
{
   render_vertex *vertexBuffer = renderer->currentVertexBuffer;
   //Offset the locked vertices from base
   u32 quadsLockedFromBase   = renderer->quadsLockedFromBase;

   u32 quadSize		 	 = sizeof(render_vertex) * 4;
   u32 drawOffset        = (quadsLockedFromBase + vertexArrayOffset) * 4; 

   render_vertex *drawBuffer = (vertexBuffer + drawOffset);
   for(int v = 0; v < 4; v++)
   {
     drawBuffer->location = vertices[v].location;
     drawBuffer->uv       = vertices[v].uv;
     drawBuffer->color    = vertices[v].color;
     drawBuffer->texture  = vertices[v].texture;

     drawBuffer++;
   }
   Assert(drawBuffer <
		  renderer->currentVertexBuffer + (renderer->MaxQuadDraws * 4));

   renderer->drawcount += 4;
   //CopyTo(DrawData, vertices, QUADSIZE);
}

inline void
opengl_swap_buffers(opengl_device *gl_device)
{
}

static void *
opengl_init(
		memory_area *area,
		void *windown_handle,
		game_renderer *renderer,
		u32 texture_array_w,
		u32 texture_array_h,
		u32 texture_capacity,
		u32 max_quad_size,
		u32 back_buffer_w,
		u32 back_buffer_h)
{
	opengl_device *gl_device = memory_area_clear_and_push_struct(
			area, opengl_device);
	return(gl_device);
}

inline void
opengl_clear(opengl_device *gl_device, f32 clear_color[4])
{
	glClearColor(
			clear_color[0],
			clear_color[1],
			clear_color[2],
			clear_color[3]
			);
	glClear(GL_COLOR_BUFFER_BIT);
}

typedef struct{
    u32 totalDrawCount;
    u32 indexDrawCount;
    u32 vertexDrawStart;

	//total quads in the restorable vertex buffer
    u32 pushedQuadsToBuffer;

	u32 reservedLockedGroupsCount;
	u32 *reservedLockedGroups;

	//for the scissors command
	u16 scissorsOnStack;
	u16 scissorCurrent;
	u32 scissorPushCount;

	rectangle32s currentDrawClip;

}gl_render_op;

static void
opengl_read_commands(opengl_device *gl_device,
                game_renderer *renderer,
                render_commands *rendercommands,
                gl_render_op *batch,
				u32 processingPeeling)
{

#if 0
   u32 errorTestIndex = 0;
   u8 *command = rendercommands->commands_base;
   rectangle32s currentClip = {0};
   while(command != rendercommands->commands_offset)
   {
       errorTestIndex++;
       render_command_type *renderheader = (render_command_type *)command;
       //Get the data here.
       u32 offset = 0;
       switch(*renderheader)
       {
           case render_command_type_clear:
               {
                   render_command_clear *data = (render_command_clear *)command;
                   offset = sizeof(render_command_clear);
                   opengl_clear(gl_device, renderer->clearcolor);
                   //Clear screen
               }break;
           case render_command_type_drawquad:
               {
                   render_command_drawquad *data = (render_command_drawquad *)command;
                   offset = sizeof(render_command_drawquad);
                   //DrawQuad
                   d3d_PushQuad(renderer, data->vertices, batch->pushedQuadsToBuffer); 
				   batch->pushedQuadsToBuffer++;
                   batch->indexDrawCount++;

               }break;

           case render_command_type_PushClip:
               {
                   render_command_SetClip *data = (render_command_SetClip *)command;
                   offset = sizeof(render_command_SetClip);

				   //Get the first scissor from the stack
				   render_scissor *scissorPushed = renderer->scissorStack + batch->scissorPushCount;
				   i32 cX0 = scissorPushed->clip.x0;
				   i32 cY0 = scissorPushed->clip.y0;
				   i32 cX1 = scissorPushed->clip.x1;
				   i32 cY1 = scissorPushed->clip.y1;

				   batch->scissorCurrent = batch->scissorPushCount;
				   batch->scissorsOnStack++;
				   batch->scissorPushCount++;
				   
				   //;Cleanup
					u32 tempScissorStackCount = renderer->scissorTotalCount; 
				    Assert(batch->scissorsOnStack < tempScissorStackCount);
				   //
				   //Draw current quads before switching clip.
				   //
                   d3d_DrawIndexed(gl_device, batch);
				   d3d_SetClip(gl_device, cX0, cY0, cX1, cY1);

				   currentClip.x0 = cX0;
				   currentClip.y0 = cY0;
				   currentClip.x1 = cX1;
				   currentClip.y1 = cY1;
                   //DrawQuad

               }break;
		   case render_command_type_PopClip:
			   {
                    offset = sizeof(render_command_PopClip);
					batch->scissorsOnStack--;

					u32 tempScissorStackCount = renderer->scissorTotalCount; 
				    Assert(batch->scissorsOnStack < tempScissorStackCount);
				    //
				    //Restore clip
				    //Pop stack?
					i32 cX0 = 0; 
					i32 cY0 = 0; 
					i32 cX1 = 0; 
					i32 cY1 = 0; 

				    render_scissor *scissorStack = renderer->scissorStack;
					if(batch->scissorsOnStack > 0)
					{
					   batch->scissorCurrent = scissorStack[batch->scissorCurrent].previous;
					   u32 sI = batch->scissorCurrent;
					   cX0 = scissorStack[sI].clip.x;
					   cY0 = scissorStack[sI].clip.y;
					   //These where already clipadded before.
					   cX1 = scissorStack[sI].clip.w;
					   cY1 = scissorStack[sI].clip.h;


					}
					else
					{
				       cX0 = batch->currentDrawClip.x; 
   				       cY0 = batch->currentDrawClip.y;
   				       cX1 = batch->currentDrawClip.w;
   				       cY1 = batch->currentDrawClip.h;
					}
                    d3d_DrawIndexed(gl_device, batch);
				    d3d_SetClip(gl_device, cX0, cY0, cX1, cY1);

					currentClip.x0 = cX0;
					currentClip.y0 = cY0;
					currentClip.x1 = cX1;
					currentClip.y1 = cY1;


			   }break;
		   case render_command_type_draw_locked_vertices:
			   {
				   offset = sizeof(render_command_draw_locked_vertices_data);
                   render_command_draw_locked_vertices_data *data = (render_command_draw_locked_vertices_data *)command;

				   u32 lockGroupIndex = data->groupIndex;

				   Assert(lockGroupIndex < renderer->pushedLocksCount);

				   render_locked_vertices_group *lockedGroup = renderer->vertexLockGroups + data->groupIndex;


				   if(!processingPeeling)
				   {
				       //Draw previous vertices (+1 extra draw call)
                       d3d_DrawIndexed(gl_device, batch);

				       d3d_draw_indexed_offset(gl_device, lockedGroup->offset, lockedGroup->count);
				   }
				   else
				   {
					   batch->reservedLockedGroups[batch->reservedLockedGroupsCount++] = lockGroupIndex;
				   }
			   }break;
           default:
               {
                   Assert(0);
               }
       }
       command += offset;
       offset = 0;
   }
   Assert(errorTestIndex == rendercommands->CommandCount);
    //TODO:Get data and header in order to execute the given command.
#endif
}

inline void
opengl_draw_begin()
{
}

inline void
opengl_draw_end(opengl_device *gl_device)
{
}
