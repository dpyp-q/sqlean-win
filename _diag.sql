.load dist/time
select 'td', hex(time_date(2024,1,15));
select 'tt', typeof(time_date(2024,1,15));
create table t90_events(at blob);
insert into t90_events values (null), (time_date(2024,1,15)), (time_date(2024,3,2));
select 'cnt', count(*) from t90_events;
select 'hex', hex(at) from t90_events;
create index t90_events_year on t90_events(time_get_year(at));
select 'idxok', 1;
select 'y2024', count(*) from t90_events where time_get_year(at) = 2024;
