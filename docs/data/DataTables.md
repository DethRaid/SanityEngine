Sanity Engine will have a system that's functionally equivalent to UE4's data tables, but much better in the ways I care about

## SQL 

Data tables will probably be implemented in an SQL database. I've noticed that tables often need to have references to other tables, which is a problem SQL solves

## Multi-user edits

Edits to the tables would be stored as SQL statements. This allows multiple people to make edits to the same database. As long as they don't edit the same fields in the same rows, we'll be able to apply those SQL statements when you sync someone else's work

This might not happen for a while - it's a lot of complexity - but once Sanity Engine is being used for a multi-developer project, this kind of system will become incredibly necessary
