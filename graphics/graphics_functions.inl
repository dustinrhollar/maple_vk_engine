
#if !defined(GRAPHICS_EXPORTED_FUNCTION)
#define GRAPHICS_EXPORTED_FUNCTION( fun )
#endif

// Initialization Functions

GRAPHICS_EXPORTED_FUNCTION( initialize_graphics )
GRAPHICS_EXPORTED_FUNCTION( shutdown_graphics   )
GRAPHICS_EXPORTED_FUNCTION( set_render_mode     )
GRAPHICS_EXPORTED_FUNCTION( get_render_mode     )
// Frame Functions

GRAPHICS_EXPORTED_FUNCTION( begin_frame         )
GRAPHICS_EXPORTED_FUNCTION( end_frame           )
GRAPHICS_EXPORTED_FUNCTION( wait_for_last_frame )

// Command List Functions

GRAPHICS_EXPORTED_FUNCTION( create_command_pool )
GRAPHICS_EXPORTED_FUNCTION( create_command_list )

GRAPHICS_EXPORTED_FUNCTION( free_command_pool    )
GRAPHICS_EXPORTED_FUNCTION( free_command_list    )
GRAPHICS_EXPORTED_FUNCTION( execute_command_list )

// Gpu Resource Functions


// Pipeline Functions

GRAPHICS_EXPORTED_FUNCTION( create_pipeline )
GRAPHICS_EXPORTED_FUNCTION( free_pipeline   )

// Renderer Functions 

GRAPHICS_EXPORTED_FUNCTION( create_render_component   )
GRAPHICS_EXPORTED_FUNCTION( free_render_component     )
GRAPHICS_EXPORTED_FUNCTION( set_render_component_info )

// Upload Buffer Functions

GRAPHICS_EXPORTED_FUNCTION( create_upload_buffer   )
GRAPHICS_EXPORTED_FUNCTION( free_upload_buffer     )
GRAPHICS_EXPORTED_FUNCTION( resize_upload_buffer   )
GRAPHICS_EXPORTED_FUNCTION( copy_upload_buffer     )
GRAPHICS_EXPORTED_FUNCTION( get_upload_buffer_info )
GRAPHICS_EXPORTED_FUNCTION( update_upload_buffer   )
GRAPHICS_EXPORTED_FUNCTION( map_upload_buffer      )
GRAPHICS_EXPORTED_FUNCTION( unmap_upload_buffer    )

// Image Functions

GRAPHICS_EXPORTED_FUNCTION( create_image         )
GRAPHICS_EXPORTED_FUNCTION( free_image           )
GRAPHICS_EXPORTED_FUNCTION( resize_image         )
GRAPHICS_EXPORTED_FUNCTION( copy_buffer_to_image )
GRAPHICS_EXPORTED_FUNCTION( get_image_dimensions )


// Descriptor Functionss

GRAPHICS_EXPORTED_FUNCTION( create_descriptor_set_layout   )
GRAPHICS_EXPORTED_FUNCTION( free_descriptor_set_layout    )
GRAPHICS_EXPORTED_FUNCTION( create_descriptor_set         )
GRAPHICS_EXPORTED_FUNCTION( free_descriptor_set           )
GRAPHICS_EXPORTED_FUNCTION( bind_buffer_to_descriptor_set )

// Command Functions

GRAPHICS_EXPORTED_FUNCTION( cmd_bind_pipeline         )
GRAPHICS_EXPORTED_FUNCTION( cmd_draw                  )
GRAPHICS_EXPORTED_FUNCTION( cmd_set_object_world_data )
GRAPHICS_EXPORTED_FUNCTION( cmd_set_camera            )
GRAPHICS_EXPORTED_FUNCTION( cmd_bind_descriptor_set   )

#undef GRAPHICS_EXPORTED_FUNCTION
