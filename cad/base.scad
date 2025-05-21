$fn = 200;
plate_thickness = 2;  
plate_length = 126;  
plate_width = 106; 
border_thickness = 1.4;

module load_cell(with_screew, with_bottom_cut)
{
    length = 45;
    width = 10;
    height = 8.5;
    difference()
    {
        cube([length, width, height], center = false);
        if (with_screew)
            union()
            {
                for (x = [4, 4 + 37, 11.5, 11.5 + 22])
                   for (y = [5])
                   {
                       translate([x, y, -0.5])
                           cylinder(h = height + 1, d = 3.0, center = false);
                   }
            }
    }
    if (with_screew)
    {
        for (x = [4, 4 + 37, 11.5, 11.5 + 22])
            translate([0, width, height])
            rotate([180, 0, 0])
            for (y = [5])
            {
                translate([x, y, 0 - plate_thickness])
                    cylinder(h = height + plate_thickness, d = 3.0, center = false);
                translate([x, y, 0 - plate_thickness])
                cylinder(h = plate_thickness + 0.1, d1 = 5.5, d2 = 3.1, center = false); // Countersunk hole
            }
    }
    if (with_bottom_cut)
    {
        tolerance = 1.5;
        translate([13.5, -tolerance, 0 - plate_thickness])
            cube([length - 13.5 + tolerance, width + tolerance * 2, plate_thickness + 5 + 0.1], center = false);
    }
}

module screen(mounting_tall, generate_screen)
{
    length = 35.4;
    width = 33.5;
    translate([1, 1, 0])
    difference()
    {
        screen_plate_thickness = 5;
        union()
        {
            translate([0, 0, mounting_tall])
                if (generate_screen)
                {
                    cube([length, width, screen_plate_thickness], center = false);
                    translate([3.04, 7.25, screen_plate_thickness])
                        cube([29.42, 14.7, 1.5], center = false);
                }
            for (x = [length - 6, 0])
               for (y = [width  - 6,  0])
               {
                   translate([x, y, 0])
                       cube([6, 6, mounting_tall]);
               }
        }
        union()
        {
           // Add holes at every corner
           for (x = [length  - 2.5, 2.5])
               for (y = [width  - 2.5,  2.5])
               {
                   translate([x, y, -0.5])
                       cylinder(h = plate_thickness + 1 + mounting_tall, d = 2.9, center = false);
               }
        }
    }
}

module only_base(length, width, thickness, without_center)
{
    cube([length, width, thickness], center = false);
    translate([length /2 , width - 38, 0])
        cylinder(h = thickness, d = 92, center = false);
}

module xh711(mounting_tall, generate_body)
{
    length = 35;
    width = 21;
    height = 4;
    rotate([0, 0, 180])
        difference()
        {
            union()
            {
                if (generate_body)
                {
                    translate([0, 0, mounting_tall])
                        cube([length, width, height], center = false);
                }
                for (x = [length - 8])
                    for (y = [width - 5.5,  0])
                    {
                        translate([x, y, 0])
                            cube([8, 5.5, mounting_tall]);
                    }
            }
            union()
            {
               // Add holes at every corner
               for (x = [length  - 3.5])
                   for (y = [width  - 2.5,  2.5])
                   {
                       translate([x, y, -0.5])
                           cylinder(h = plate_thickness + 1 + mounting_tall, d = 2.9, center = false);
                   }
            }
        }
}

module battery()
{
    translate([0, 0, plate_thickness])
        cube([60, 20, 4]);
}

module esp32(mounting_tall, only_holder, usb_factor=1)
{
    usb_length = 7;  // Length of the port
    usb_height = 3.4 * usb_factor;  // Height of the port
    usb_thickness = 9 * usb_factor;  // Thickness of the port
    corner_radius = 0.6;  // Radius of the rounded corners

board_length = 21;
      board_width = 17.8;
      board_thickness = 1.3;
      holder_thickness = 1.3;
      clip_height = 4;

    if (!only_holder)
    {
        // Draw USB Type-C port
        translate([1, 0, 0])
        rotate([0, 90, 0])
          minkowski() {
            cube([usb_height-corner_radius*2, usb_thickness- corner_radius*2, usb_length/2], center=true);
            cylinder(h=usb_length/2, r=corner_radius, center=true);
          }
        translate([-8, 0, -2.2])
        cube([board_length, board_width, board_thickness], center=true);
      }
    else
    {     
        for (i = [-1]) {
            translate([3.2 + i * (board_length + holder_thickness), 0, 0 - mounting_tall + 0.5])
                union()
                {
                    cube([holder_thickness, board_width, clip_height], center = true);
                    translate([holder_thickness*0.5, 0, (mounting_tall - clip_height - board_thickness)/2])
                    {
                        cube([holder_thickness*0.5, board_width, mounting_tall - board_thickness], center = true);
                        translate([-0.3, 0, 2.6])
                            rotate([90, 0, 0])
                                cylinder(h=board_width, r=0.5, center=true);
                    }
                }
        }
    }
}

module button(only_holder)
{
    width = 12.05;
    length = 22;
    height = 3;
    holder_thickness = 1;
    if (!only_holder)
    {
    cube([length, width, height], center = false);
        translate([length / 2, width / 2, height])
            cylinder(h = 9, d = 12.5, center = false);
    }
    else
    {
    translate([length / 2 - length/8, -holder_thickness, 0])
        cube([length/4, holder_thickness, height+holder_thickness], center = false);
    translate([length / 2 - length/8, 0, height + 0.25])
        rotate([0, 90, 0])
            cylinder(h = length/4, d = 0.5, center = false);
        
    translate([length / 2 - length/8, width, 0])
        cube([length/4, holder_thickness, height+holder_thickness], center = false);
    translate([length / 2 - length/8, width, height + 0.25])
        rotate([0, 90, 0])
            cylinder(h = length/4, d = 0.5, center = false);
    }
}


module base_plate()
{
    // Plate dimensions  
    border_height = 10.5;

    difference()
    {
        union()
        {
            translate([-1, -1, 0])
                difference()
                {
                    only_base(plate_length + border_thickness * 2, plate_width + border_thickness * 2, border_height);
                    translate([1, 1, 0])
                        only_base(plate_length, plate_width, border_height);
                }
            only_base(plate_length, plate_width, plate_thickness);
            translate([plate_length /2 , plate_width - 38, plate_thickness])
                difference()
                {
                    cylinder(h = border_height - plate_thickness, d = 70 + border_thickness * 2, center = false);
                    cylinder(h = border_height - plate_thickness, d = 70, center = false);
                }
            screws(2.9, true);
        }
                
        translate([plate_length /2 , plate_width - 38, 0.5])
            cylinder(h = 1.5, d = 6, center = false);
        
        translate([plate_length /2 + 5 ,18 , plate_thickness])
            rotate([0, 0, 90])
                load_cell(true, true); 
     
        translate([plate_length /2 -6.5 ,25 , plate_thickness])
            cube(13);

        
        translate([2, 55, 6])
            rotate([0, 0, 180])
                esp32(3, false);
        
        translate([53, 1, plate_thickness - 1])
            rotate([0, 0, 90])
                button();
        
        translate([85, 1, plate_thickness - 1])
            rotate([0, 0, 90])
                button();
        
        screws(2.9);
    }
    

 
    
    translate([2.6, 55, 6])
        rotate([0, 0, 180])
            esp32(3, true);
    //translate([plate_length /2 + 5 ,18 , plate_thickness])
    //    rotate([0, 0, 90])
    //        load_cell(false, false);  
    
    screen(5, false);
    
    translate([124, 22, 0])
        xh711(4, false);
    
    // TODO: false
    translate([53, 1, plate_thickness - 1])
            rotate([0, 0, 90])
                button(false);
    // TODO: false
    translate([85, 1, plate_thickness - 1])
        rotate([0, 0, 90])
            button(false);
    
    //translate([125, 40, 0])
    //    rotate([0, 0, 90])
    //        battery();
    
    
    
}

module top_cover_lid(width, length, tolerance, border_height, cover_thickness, diameter)
{    
    difference()
            {
                translate([- tolerance /2 - cover_thickness*2, - tolerance / 2  - cover_thickness*2, -border_height])
                    only_base(length + tolerance + cover_thickness*4, width + tolerance + cover_thickness * 4, border_height);
                translate([length /2 , width - 38, -border_height- cover_thickness])
                    cylinder(h = border_height * 2, d = diameter - tolerance - plate_thickness * 2 , center = false);
            }
}

module top_cover()
{
    
    tolerance = 0.4;

    border_height = 16;
    translate([0, 0, 12.5])
        union()
        {
            difference()
            {
                top_cover_lid(plate_width, plate_length, tolerance, border_height + 5, border_thickness, 70);             
                translate([0, 0, -plate_thickness])
                    top_cover_lid(plate_width, plate_length, 0, border_height + 10, border_thickness / 2, 72);
                translate([-border_thickness , tolerance, - border_height -plate_thickness - 8.5])
                    cube([plate_length + border_thickness*2, plate_width*2 + border_thickness, border_height]);
            }
        }
        
}

module screw(diameter, only_mounting)
{    
    if (!only_mounting)
    {
        cylinder(h = plate_thickness + 0.1, d1 = 5.5, d2 = 3.1, center = false); 
        translate([0, 0, plate_thickness])
            cylinder(h = plate_thickness *3, d=diameter, center = false); 
    }
    else
    {
        translate([0, 0, plate_thickness])
            cylinder(h = plate_thickness *2, d=diameter + 1, center = false); 
    }
}


module screws(diameter, only_mounting)
{
    height = 3;
    translate([-border_thickness*2 - 0.4, 10, height])
        rotate([0, 90, 0])
            screw(diameter, only_mounting);
    
    translate([-border_thickness*2 - 0.4, 90, height])
        rotate([0, 90, 0])
            screw(diameter, only_mounting);
    
    translate([plate_length + border_thickness * 2 + 0.4, 10, height])
        rotate([0, -90, 0])
            screw(diameter, only_mounting);
    
    translate([plate_length  + border_thickness * 2 + 0.4, 90, height])
        rotate([0, -90, 0])
            screw(diameter, only_mounting);
}

module lid()
{
    difference()
    {
        top_cover();
        base_plate();
        screen(6, true);
        translate([plate_length /2 + 5 ,18 , plate_thickness  ])
            rotate([0, 0, 90])
                load_cell(false, false);
        translate([border_thickness, 55, 6])
            rotate([0, 0, 180])
                esp32(3, false, 1.8);
        
        translate([plate_length - 19 , 0, -9.5])
            rotate([90, 0, 0])
                cylinder(h = border_thickness*4, d = 17, center=false); 
        screws(3.1);
        translate([85, 9, 12])
            linear_extrude()
                text("⏼", font = "Segoe UI Symbol", size = 7);
        translate([54, 9, 12])
            linear_extrude()
                text("0", font = "Segoe UI Symbol", size = 7);
    }

    //screws(3.1);

}

module scale_plate()
{
    
    difference()
    {
        union()
        {
            translate([25, 25, 10.5])
                cube([80, 80, plate_thickness]);
            for (x = [35, 40, 45, 50, 55, 70, 75, 80, 85, 90])
                translate([x, 25, 8.5])
                    cube([1.5, 80, 3]);
            for (x = [60, 65])
                translate([x, 71, 8.5])
                    cube([1.5, 50, 3]);
        }
        difference()
        {
            translate([plate_length /2 , plate_width - 38, 5.5])
                cylinder(h = plate_thickness + 5, d = 150 , center = false);
            translate([plate_length /2 , plate_width - 38, 5.5])
                cylinder(h = plate_thickness + 5, d = 63 , center = false);
        }
        translate([plate_length /2 + 5 ,18 , plate_thickness  ])
            rotate([0, 0, 90])
                load_cell(true, false);
        translate([plate_length /2 , plate_width - 38, 10])
            cylinder(h = 1.5, d = 6, center = false);
        
        translate([plate_length /2 + 5 ,4 , plate_thickness + 0.5  ])
            rotate([0, 0, 90])
                load_cell(false, false);
    }
}


//base_plate();

lid();


//scale_plate();

